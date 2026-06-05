#include <pch.h>
#include "MetalPBRMaterial.h"

#include <Metal/Metal.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/Renderer.h>
#include <Cabrankengine/Renderer/Shader.h>
#include <Common/BinaryFormats.h>

#include "MetalBinding.h"
#include "MetalDeviceContext.h"
#include "MetalShader.h"
#include "MetalStorageBuffer.h"

namespace cbk::platform::metal {

	using namespace rendering;

	bool MetalPBRMaterial::s_Initialized = false;
	MTL::RenderPipelineState* MetalPBRMaterial::s_Pipeline = nullptr;
	MTL::DepthStencilState* MetalPBRMaterial::s_DepthState = nullptr;

	MetalPBRMaterial::MetalPBRMaterial() : PBRMaterial() {
		initSharedResourcesIfNeeded();
	}

	MetalPBRMaterial::~MetalPBRMaterial() = default;

	void MetalPBRMaterial::record(MTL::RenderCommandEncoder* encoder, const math::Mat4& transform) const {
		encoder->setRenderPipelineState(s_Pipeline);
		encoder->setDepthStencilState(s_DepthState);

		// Scene globals to both stages, model matrix to the vertex stage.
		encoder->setVertexBytes(Renderer::sceneUniformData(), Renderer::sceneUniformSize(), buffer_index::k_Scene);
		encoder->setFragmentBytes(Renderer::sceneUniformData(), Renderer::sceneUniformSize(), buffer_index::k_Scene);
		encoder->setVertexBytes(&transform, sizeof(math::Mat4), buffer_index::k_Push);

		// Material parameters to the fragment stage.
		PushData push{ getAlbedoColor(), getMetalness(), getRoughness() };
		encoder->setFragmentBytes(&push, sizeof(PushData), buffer_index::k_Push);

		// Point-light array: the Renderer's storage buffer is a CPU byte-holder on
		// Metal; push its bytes directly (count header + PointLight[], <4 KB).
		if (auto* lights = static_cast<MetalStorageBuffer*>(Renderer::getLightSSBO().get()))
			encoder->setFragmentBytes(lights->data(), lights->size(), buffer_index::k_Lights);

		if (m_AlbedoMap)
			m_AlbedoMap->bind(0);
		if (m_NormalMap)
			m_NormalMap->bind(1);
		if (m_MetalRoughMap)
			m_MetalRoughMap->bind(2);
		if (m_AOMap)
			m_AOMap->bind(3);
	}

	void MetalPBRMaterial::initSharedResourcesIfNeeded() {
		if (s_Initialized)
			return;
		s_Initialized = true;

		auto* context = static_cast<MetalDeviceContext*>(Application::get().getWindow().getContext());
		MTL::Device* device = context->getDevice();
		auto* shader = static_cast<MetalShader*>(ShaderLibrary::get("PBR").get());

		// Vertex layout = common::Vertex (pos3, normal3, texCoords2, tangent3 = 44 B).
		MTL::VertexDescriptor* vd = MTL::VertexDescriptor::alloc()->init();
		auto setAttr = [&](uint32_t i, MTL::VertexFormat fmt, uint32_t offset) {
			vd->attributes()->object(i)->setFormat(fmt);
			vd->attributes()->object(i)->setOffset(offset);
			vd->attributes()->object(i)->setBufferIndex(buffer_index::k_VertexData);
		};
		setAttr(0, MTL::VertexFormatFloat3, offsetof(common::Vertex, position));
		setAttr(1, MTL::VertexFormatFloat3, offsetof(common::Vertex, normal));
		setAttr(2, MTL::VertexFormatFloat2, offsetof(common::Vertex, texCoords));
		setAttr(3, MTL::VertexFormatFloat3, offsetof(common::Vertex, tangent));
		vd->layouts()->object(buffer_index::k_VertexData)->setStride(sizeof(common::Vertex));

		MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
		desc->setVertexFunction(shader->getVertexFunction());
		desc->setFragmentFunction(shader->getFragmentFunction());
		desc->setVertexDescriptor(vd);
		// Opaque. The depth format must match the texture MetalRendererAPI attaches.
		desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
		desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

		NS::Error* error = nullptr;
		s_Pipeline = device->newRenderPipelineState(desc, &error);
		if (!s_Pipeline)
			CBK_CORE_ERROR("MetalPBRMaterial: pipeline creation failed: {}",
			               error ? error->localizedDescription()->utf8String() : "unknown error");

		// Opaque depth test: pass fragments at or in front of the stored depth and
		// write their depth so later draws are occluded correctly.
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
		depthDesc->setDepthWriteEnabled(true);
		s_DepthState = device->newDepthStencilState(depthDesc);
		depthDesc->release();

		vd->release();
		desc->release();
	}

	void MetalPBRMaterial::destroySharedResources() {
		if (!s_Initialized)
			return;
		if (s_Pipeline)
			s_Pipeline->release();
		s_Pipeline = nullptr;
		if (s_DepthState)
			s_DepthState->release();
		s_DepthState = nullptr;
		s_Initialized = false;
	}

} // namespace cbk::platform::metal
