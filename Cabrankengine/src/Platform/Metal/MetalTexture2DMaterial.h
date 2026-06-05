#pragma once

#include <array>

#include <Cabrankengine/Renderer/Materials/Texture2DMaterial.h>
#include <Cabrankengine/Renderer/Texture.h>
#include <Common/Math/Mat4.h>

#include "IMetalRecordable.h"

namespace MTL {
	class RenderPipelineState;
	class RenderCommandEncoder;
	class DepthStencilState;
} // namespace MTL

namespace cbk::platform::metal {

	// Material backing the Renderer2D batch quad pipeline. Owns a shared pipeline
	// state (built from the "Texture" shader) at class scope; per-instance state is
	// the 32 texture slots and the view-projection the batcher supplies each flush.
	// Mirrors VulkanTexture2DMaterial.
	class MetalTexture2DMaterial : public rendering::Texture2DMaterial, public IMetalRecordable {
	  public:
		static constexpr uint32_t k_MaxTextureSlots = 32;

		MetalTexture2DMaterial();
		~MetalTexture2DMaterial() override;

		void bind() const override {}

		// Slot 0 is conventionally the white texture used by un-textured quads.
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
