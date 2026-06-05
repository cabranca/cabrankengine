#include <pch.h>
#include "MetalTexture2DMaterial.h"

#include <Metal/Metal.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Cabrankengine/Renderer/Shader.h>

#include "MetalBinding.h"
#include "MetalDeviceContext.h"
#include "MetalShader.h"

namespace cbk::platform::metal {

	using namespace rendering;

	// Must match QuadVertex in Renderer2D.cpp (stride 44).
	struct QuadVertex {
		float position[3];
		float color[4];
		float texCoord[2];
		float texIndex;
		float tilingFactor;
	};

	bool MetalTexture2DMaterial::s_Initialized = false;
	MTL::RenderPipelineState* MetalTexture2DMaterial::s_Pipeline = nullptr;
	MTL::DepthStencilState* MetalTexture2DMaterial::s_DepthState = nullptr;

	MetalTexture2DMaterial::MetalTexture2DMaterial() : Texture2DMaterial() {
		initSharedResourcesIfNeeded();
	}

	MetalTexture2DMaterial::~MetalTexture2DMaterial() = default;

	void MetalTexture2DMaterial::setTextureSlot(uint32_t slot, const Ref<Texture2D>& texture) {
		CBK_CORE_ASSERT(slot < k_MaxTextureSlots, "MetalTexture2DMaterial: texture slot out of range");
		m_TextureSlots[slot] = texture;
	}

	void MetalTexture2DMaterial::setViewProjection(const math::Mat4& viewProjection) {
		m_ViewProjection = viewProjection;
	}

	void MetalTexture2DMaterial::record(MTL::RenderCommandEncoder* encoder, const math::Mat4& /*transform*/) const {
		encoder->setRenderPipelineState(s_Pipeline);
		encoder->setDepthStencilState(s_DepthState);

		// Batched quads are already in world space; only the view-projection is needed.
		encoder->setVertexBytes(&m_ViewProjection, sizeof(math::Mat4), buffer_index::k_Scene);

		// Slot 0 is the white fallback for unused slots — the shader still indexes the
		// full array, so every slot must point at a valid texture.
		const auto& fallback = m_TextureSlots[0];
		for (uint32_t i = 0; i < k_MaxTextureSlots; ++i) {
			const auto& tex = m_TextureSlots[i] ? m_TextureSlots[i] : fallback;
			if (tex)
				tex->bind(i); // setFragmentTexture(m_Texture, i) on the active encoder
		}
	}

	void MetalTexture2DMaterial::initSharedResourcesIfNeeded() {
		if (s_Initialized)
			return;
		s_Initialized = true;

		auto* context = static_cast<MetalDeviceContext*>(Application::get().getWindow().getContext());
		MTL::Device* device = context->getDevice();
		auto* shader = static_cast<MetalShader*>(ShaderLibrary::get("Texture").get());

		MTL::VertexDescriptor* vd = MTL::VertexDescriptor::alloc()->init();
		auto setAttr = [&](uint32_t i, MTL::VertexFormat fmt, uint32_t offset) {
			vd->attributes()->object(i)->setFormat(fmt);
			vd->attributes()->object(i)->setOffset(offset);
			vd->attributes()->object(i)->setBufferIndex(buffer_index::k_VertexData);
		};
		setAttr(0, MTL::VertexFormatFloat3, offsetof(QuadVertex, position));
		setAttr(1, MTL::VertexFormatFloat4, offsetof(QuadVertex, color));
		setAttr(2, MTL::VertexFormatFloat2, offsetof(QuadVertex, texCoord));
		setAttr(3, MTL::VertexFormatFloat, offsetof(QuadVertex, texIndex));
		setAttr(4, MTL::VertexFormatFloat, offsetof(QuadVertex, tilingFactor));
		vd->layouts()->object(buffer_index::k_VertexData)->setStride(sizeof(QuadVertex));

		MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
		desc->setVertexFunction(shader->getVertexFunction());
		desc->setFragmentFunction(shader->getFragmentFunction());
		desc->setVertexDescriptor(vd);

		auto* color = desc->colorAttachments()->object(0);
		color->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
		// Standard src-alpha over blending so quads with alpha < 1 composite correctly.
		color->setBlendingEnabled(true);
		color->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
		color->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
		color->setRgbBlendOperation(MTL::BlendOperationAdd);
		color->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
		color->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
		color->setAlphaBlendOperation(MTL::BlendOperationAdd);

		// The pass binds a depth attachment, so the PSO must declare its format even
		// though batched quads don't test or write depth.
		desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);

		NS::Error* error = nullptr;
		s_Pipeline = device->newRenderPipelineState(desc, &error);
		if (!s_Pipeline)
			CBK_CORE_ERROR("MetalTexture2DMaterial: pipeline creation failed: {}",
			               error ? error->localizedDescription()->utf8String() : "unknown error");

		// Overlay quads composite on top in submission order: never test or write depth.
		MTL::DepthStencilDescriptor* depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
		depthDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
		depthDesc->setDepthWriteEnabled(false);
		s_DepthState = device->newDepthStencilState(depthDesc);
		depthDesc->release();

		vd->release();
		desc->release();
	}

	void MetalTexture2DMaterial::destroySharedResources() {
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
