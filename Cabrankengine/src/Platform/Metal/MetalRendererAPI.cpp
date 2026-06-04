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

#include "MetalBuffer.h"
#include "MetalDeviceContext.h"
#include "MetalShader.h"
#include "MetalVertexArray.h"

namespace cbk::platform::metal {

	using namespace rendering;

	void MetalRendererAPI::init() {
		auto& window = Application::get().getWindow();
		auto glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());

		// Investigate more on the connection between Metal and GLFW
		// --- MAGIA para conectar GLFW con Metal-cpp sin usar .mm ---
		// 1. Obtener la NSWindow (void*)
		void* nswindow = glfwGetCocoaWindow(glfwWindow);

		// 2. Obtener la contentView: [nswindow contentView]
		// Usamos objc_msgSend para llamar métodos de ObjC desde C++
		void* contentView = ((void* (*)(id, SEL))objc_msgSend)((id)nswindow, sel_registerName("contentView"));

		// 3. Configurar el Layer
		m_Swapchain = CA::MetalLayer::layer();
		m_Swapchain->setDevice(static_cast<MetalDeviceContext*>(window.getContext())->getDevice());

		// This shouldn't be hardcoded. Maybe better to ask capabilities as with Vulkan?
		m_Swapchain->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

		// 4. Asignar el layer: [view setLayer:layer] y [view setWantsLayer:YES]
		((void (*)(id, SEL, void*))objc_msgSend)((id)contentView, sel_registerName("setLayer:"), (void*)m_Swapchain);
		((void (*)(id, SEL, BOOL))objc_msgSend)((id)contentView, sel_registerName("setWantsLayer:"), YES);
		// ------------------------------------------------------------
	}

	void MetalRendererAPI::shutdown() {
		if (m_Swapchain)
			m_Swapchain->release();
	}

	void MetalRendererAPI::setClearColor(const math::Vector4& color) {
		m_ClearColor = color;
	}

	void MetalRendererAPI::clear() {
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

		s_ActiveEncoder = m_ActiveCommandBuffer->renderCommandEncoder(pass);
		pass->release();

		// Viewport and scissor must be set every frame in Metal
		MTL::Viewport viewport;
		viewport.originX = 0.0;
		viewport.originY = 0.0;
		viewport.width = (double)m_CurrentDrawable->texture()->width();
		viewport.height = (double)m_CurrentDrawable->texture()->height();
		viewport.znear = 0.0;
		viewport.zfar = 1.0;
		s_ActiveEncoder->setViewport(viewport);

		MTL::ScissorRect scissor;
		scissor.x = 0;
		scissor.y = 0;
		scissor.width = m_CurrentDrawable->texture()->width();
		scissor.height = m_CurrentDrawable->texture()->height();
		s_ActiveEncoder->setScissorRect(scissor);
	}

	void MetalRendererAPI::beginFrame() {}

	void MetalRendererAPI::draw(const Ref<GeometryDescriptor>& desc) {}

	void MetalRendererAPI::drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform,
	                                   uint32_t indexCount) {
		if (!s_ActiveEncoder || !s_CurrentShader)
			return;

		// auto* metalVA = static_cast<MetalVertexArray*>(desc.get());

		// // Ensure PSO exists — creates it lazily with the vertex descriptor from the GeometryDescriptor
		// s_CurrentShader->ensurePipelineState(metalVA->getVertexDescriptor());

		// MTL::RenderPipelineState* pso = s_CurrentShader->getPipelineState();
		// if (!pso)
		// 	return;

		// s_ActiveEncoder->setRenderPipelineState(pso);

		// // Push pending uniform buffers onto the encoder (setMat4, setFloat4, etc.)
		// for (const auto& [bufferIndex, data] : s_CurrentShader->getPendingVertexBytes()) {
		// 	s_ActiveEncoder->setVertexBytes(data.data(), data.size(), bufferIndex);
		// }

		// // Set vertex buffers on the encoder
		// const auto& vbs = metalVA->getVertexBuffers();
		// for (uint32_t i = 0; i < vbs.size(); i++) {
		// 	auto* metalVB = static_cast<MetalVertexBuffer*>(vbs[i].get());
		// 	s_ActiveEncoder->setVertexBuffer(metalVB->getBuffer(), 0, i);
		// }

		// // Draw with index buffer
		// auto* metalIB = static_cast<MetalIndexBuffer*>(metalVA->getIndexBuffer().get());
		// uint32_t count = indexCount == 0 ? metalIB->getCount() : indexCount;

		// s_ActiveEncoder->drawIndexedPrimitives(
		// 	MTL::PrimitiveTypeTriangle,
		// 	(NS::UInteger)count,
		// 	MTL::IndexTypeUInt32,
		// 	metalIB->getBuffer(),
		// 	(NS::UInteger)0
		// );
	}

	void MetalRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {}

	void MetalRendererAPI::SetCurrentShader(MetalShader* shader) {
		s_CurrentShader = shader;
	}

	void MetalRendererAPI::endFrame() {
		if (s_ActiveEncoder) {
			s_ActiveEncoder->endEncoding();
			s_ActiveEncoder->release();
			s_ActiveEncoder = nullptr;
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

	MTL::RenderCommandEncoder* MetalRendererAPI::GetActiveEncoder() {
		return s_ActiveEncoder;
	}
} // namespace cbk::platform::metal
