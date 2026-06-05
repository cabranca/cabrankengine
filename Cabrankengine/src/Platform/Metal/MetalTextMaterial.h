#pragma once

#include <array>

#include <Cabrankengine/Renderer/Materials/TextMaterial.h>
#include <Cabrankengine/Renderer/Texture.h>
#include <Common/Math/Mat4.h>

#include "IMetalRecordable.h"

namespace MTL {
	class RenderPipelineState;
	class RenderCommandEncoder;
	class DepthStencilState;
} // namespace MTL

namespace cbk::platform::metal {

	// Material backing the TextRenderer glyph pipeline. Mirrors MetalTexture2DMaterial
	// except the vertex layout lacks tilingFactor and the fragment shader treats the
	// sampled red channel as coverage. Mirrors VulkanTextMaterial.
	class MetalTextMaterial : public rendering::TextMaterial, public IMetalRecordable {
	  public:
		static constexpr uint32_t k_MaxTextureSlots = 32;

		MetalTextMaterial();
		~MetalTextMaterial() override;

		void bind() const override {}

		void setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) override;
		void setViewProjection(const math::Mat4& viewProjection) override;

		// IMetalRecordable
		void record(MTL::RenderCommandEncoder* encoder, const math::Mat4& transform) const override;

		// Per-class pipeline cleanup. Called by MetalRendererAPI::shutdown().
		static void destroySharedResources();

	  private:
		static void initSharedResourcesIfNeeded();

		static bool s_Initialized;
		static MTL::RenderPipelineState* s_Pipeline;
		// Overlay: depth test and write disabled (composites on top, like Vulkan).
		static MTL::DepthStencilState* s_DepthState;

		std::array<Ref<rendering::Texture2D>, k_MaxTextureSlots> m_TextureSlots{};
		math::Mat4 m_ViewProjection{};
	};

} // namespace cbk::platform::metal
