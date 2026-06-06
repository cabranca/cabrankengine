#include <pch.h>
#include "MetalRendererAPI.h"

#include <objc/runtime.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/GeometryDescriptor.h>
#include <Cabrankengine/Renderer/Materials/Material.h>

#include "IMetalRecordable.h"
#include "MetalDeviceContext.h"
#include "MetalGeometryDescriptor.h"
#include "MetalPBRMaterial.h"
#include "MetalPhongMaterial.h"
#include "MetalTextMaterial.h"
#include "MetalTexture2DMaterial.h"

namespace cbk::platform::metal {

	using namespace rendering;

	void MetalRendererAPI::init() {
		auto& window = Application::get().getWindow();
		auto glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());

		// Connect GLFW (Cocoa) to Metal without an Objective-C++ (.mm) file: fetch the
		// NSWindow's contentView and attach a CAMetalLayer to it via objc_msgSend.
		void* nswindow = glfwGetCocoaWindow(glfwWindow);
		void* contentView = ((void* (*)(id, SEL))objc_msgSend)((id)nswindow, sel_registerName("contentView"));

		m_Swapchain = CA::MetalLayer::layer();
		m_Swapchain->setDevice(static_cast<MetalDeviceContext*>(window.getContext())->getDevice());

		// drawableSize is in *physical pixels*, not logical points. On a Retina display
		// the window is e.g. 1280x720 points but the backing store is 2560x1440 pixels,
		// so query the framebuffer size — using the point size would render at half
		// resolution (upscaled by Core Animation) and, critically, would mismatch ImGui's
		// HiDPI viewport (DisplaySize * DisplayFramebufferScale), making its UI 2x too big.
		int fbWidth = 0;
		int fbHeight = 0;
		glfwGetFramebufferSize(glfwWindow, &fbWidth, &fbHeight);
		m_Swapchain->setDrawableSize(CGSize{ .width = static_cast<double>(fbWidth), .height = static_cast<double>(fbHeight) });

		// This shouldn't be hardcoded. Maybe better to ask capabilities as with Vulkan?
		m_Swapchain->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
		m_CurrentDrawable = m_Swapchain->nextDrawable();

		((void (*)(id, SEL, void*))objc_msgSend)((id)contentView, sel_registerName("setLayer:"), (void*)m_Swapchain);
		((void (*)(id, SEL, BOOL))objc_msgSend)((id)contentView, sel_registerName("setWantsLayer:"), YES);
	}

	void MetalRendererAPI::shutdown() {
		// Release each material class's shared pipeline state while the device is alive.
		MetalTexture2DMaterial::destroySharedResources();
		MetalTextMaterial::destroySharedResources();
		MetalPhongMaterial::destroySharedResources();
		MetalPBRMaterial::destroySharedResources();

		if (m_DepthTexture)
			m_DepthTexture->release();

		if (m_Swapchain)
			m_Swapchain->release();
	}

	void MetalRendererAPI::setClearColor(const math::Vector4& color) {
		m_ClearColor = color;
	}

	void MetalRendererAPI::beginFrame() {
		const auto& window = Application::get().getWindow();
		MetalDeviceContext* context = static_cast<MetalDeviceContext*>(window.getContext());

		MTL::CommandQueue* queue = context->getCommandQueue();
		m_ActiveCommandBuffer = queue->commandBuffer();

		MTL::RenderPassDescriptor* pass = MTL::RenderPassDescriptor::renderPassDescriptor();
		auto colorAtt = pass->colorAttachments()->object(0);
		colorAtt->setTexture(m_CurrentDrawable->texture());
		colorAtt->setLoadAction(MTL::LoadActionClear);
		colorAtt->setClearColor(MTL::ClearColor::Make(m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]));
		colorAtt->setStoreAction(MTL::StoreActionStore);

		// Depth attachment. The depth texture must match the color target's dimensions,
		// so recreate it whenever the drawable resizes. Cleared to the far plane (1.0)
		// and not stored — nothing samples depth after the pass (mirrors the Vulkan
		// backend's DONT_CARE store). Depth32Float here must match the format every
		// material PSO declares via setDepthAttachmentPixelFormat.
		const uint32_t width = m_CurrentDrawable->texture()->width();
		const uint32_t height = m_CurrentDrawable->texture()->height();
		if (!m_DepthTexture || m_DepthWidth != width || m_DepthHeight != height) {
			if (m_DepthTexture)
				m_DepthTexture->release();

			MTL::TextureDescriptor* depthDesc = MTL::TextureDescriptor::alloc()->init();
			depthDesc->setTextureType(MTL::TextureType2D);
			depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
			depthDesc->setWidth(width);
			depthDesc->setHeight(height);
			depthDesc->setUsage(MTL::TextureUsageRenderTarget);
			depthDesc->setStorageMode(MTL::StorageModePrivate);
			m_DepthTexture = context->getDevice()->newTexture(depthDesc);
			depthDesc->release();

			m_DepthWidth = width;
			m_DepthHeight = height;
		}

		auto* depthAtt = pass->depthAttachment();
		depthAtt->setTexture(m_DepthTexture);
		depthAtt->setLoadAction(MTL::LoadActionClear);
		depthAtt->setClearDepth(1.0);
		depthAtt->setStoreAction(MTL::StoreActionDontCare);

		m_ActiveEncoder = m_ActiveCommandBuffer->renderCommandEncoder(pass);

		// Keep the pass descriptor alive for the whole frame: ImGui_ImplMetal_NewFrame
		// (called from ImGuiLayer::begin, after beginFrame) reads its color/depth pixel
		// formats and sample count to build its pipeline state. Released in endFrame().
		m_RenderPassDescriptor = pass;

		// Viewport and scissor must be set every frame in Metal
		MTL::Viewport viewport;
		viewport.originX = 0.0;
		viewport.originY = 0.0;
		viewport.width = (double)m_CurrentDrawable->texture()->width();
		viewport.height = (double)m_CurrentDrawable->texture()->height();
		viewport.znear = 0.0;
		viewport.zfar = 1.0;
		m_ActiveEncoder->setViewport(viewport);

		MTL::ScissorRect scissor;
		scissor.x = 0;
		scissor.y = 0;
		scissor.width = m_CurrentDrawable->texture()->width();
		scissor.height = m_CurrentDrawable->texture()->height();
		m_ActiveEncoder->setScissorRect(scissor);
	}

	void MetalRendererAPI::draw(const Ref<GeometryDescriptor>& desc) {}

	void MetalRendererAPI::drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform,
	                                   uint32_t indexCount) {
		if (!m_ActiveEncoder)
			return;

		auto* recordable = dynamic_cast<IMetalRecordable*>(material.get());
		CBK_CORE_ASSERT(recordable, "MetalRendererAPI: material does not implement IMetalRecordable");
		if (!recordable)
			return;

		auto* metalDesc = static_cast<MetalGeometryDescriptor*>(desc.get());

		// The material sets the PSO, pushes uniform bytes and binds textures; geometry
		// binds its vertex buffer; the draw reads indices from the same buffer at its
		// offset (mirrors VulkanRendererAPI::commitRenderCommands).
		recordable->record(m_ActiveEncoder, transform);
		metalDesc->bindBuffers(m_ActiveEncoder);

		uint32_t count = indexCount == 0 ? metalDesc->getIndexCount() : indexCount;
		m_ActiveEncoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle, (NS::UInteger)count, MTL::IndexTypeUInt32,
		                                       metalDesc->getBuffer(), (NS::UInteger)metalDesc->getIndexBufferOffset());
	}

	void MetalRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

	void MetalRendererAPI::endFrame() {
		if (m_ActiveEncoder) {
			m_ActiveEncoder->endEncoding();
			m_ActiveEncoder->release();
			m_ActiveEncoder = nullptr;
		}

		// Held since beginFrame() so ImGui could read it; the encoder is done with it now.
		if (m_RenderPassDescriptor) {
			m_RenderPassDescriptor->release();
			m_RenderPassDescriptor = nullptr;
		}

		// Check if this is needed.
		if (m_CurrentDrawable) {
			m_CurrentDrawable->release();
			m_CurrentDrawable = nullptr;
		}

		if (m_ActiveCommandBuffer) {
			m_CurrentDrawable = m_Swapchain->nextDrawable();

			m_ActiveCommandBuffer->presentDrawable(m_CurrentDrawable);
			m_ActiveCommandBuffer->commit();
			m_ActiveCommandBuffer->release();
			m_ActiveCommandBuffer = nullptr;
		}
	}

	MTL::RenderPassDescriptor* MetalRendererAPI::getRenderPassDescriptor() const {
		return m_RenderPassDescriptor;
	}

	MTL::CommandBuffer* MetalRendererAPI::getCommandBuffer() const {
		return m_ActiveCommandBuffer;
	}

	MTL::RenderCommandEncoder* MetalRendererAPI::getCommandEncoder() const {
		return m_ActiveEncoder;
	}
} // namespace cbk::platform::metal
