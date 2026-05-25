#pragma once

#include <array>

#include <volk/volk.h>

#include <Cabrankengine/Renderer/Materials/TextMaterial.h>
#include <Cabrankengine/Renderer/Texture.h>

#include "IVulkanRecordable.h"

namespace cbk::platform::vk {

	// Material backing the TextRenderer batch pipeline. Mirrors VulkanTexture2DMaterial
	// except the vertex layout lacks tilingFactor (glyphs sample 1:1) and the fragment
	// shader treats the sampled red channel as coverage to modulate the vertex alpha.
	class VulkanTextMaterial : public rendering::TextMaterial, public IVulkanRecordable {
	  public:
		static constexpr uint32_t k_MaxTextureSlots = 32;

		VulkanTextMaterial();
		~VulkanTextMaterial() override;

		void bind() const override {}

		// Slot 0 is conventionally the white texture used as a fallback for unused
		// slots — callers must populate slot 0 at least once before the first flush.
		void setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) override;

		// View-projection comes from the global scene UBO bound by the renderer.
		void setViewProjection(const math::Mat4& /*viewProjection*/) override {}

		// IVulkanRecordable
		[[nodiscard]] VkPipeline getPipeline() const override {
			return s_Pipeline;
		}
		[[nodiscard]] VkPipelineLayout getPipelineLayout() const override {
			return s_PipelineLayout;
		}
		[[nodiscard]] const VkDescriptorSet* getDescriptorSet() const override {
			return &m_DescriptorSet;
		}
		void recordCommandBuffer(VkCommandBuffer cb, VkPipelineLayout layout) const override;

		// Per-class pipeline state cleanup. Called by VulkanRendererAPI::shutdown().
		static void destroySharedResources();

	  private:
		static void initSharedResourcesIfNeeded();
		void        updateDescriptorSet() const;

		static bool                  s_Initialized;
		static VkDescriptorSetLayout s_DescriptorSetLayout;
		static VkDescriptorPool      s_DescriptorPool;
		static VkPipelineLayout      s_PipelineLayout;
		static VkPipeline            s_Pipeline;

		std::array<Ref<rendering::Texture2D>, k_MaxTextureSlots> m_TextureSlots{};
		VkDescriptorSet                                          m_DescriptorSet{ VK_NULL_HANDLE };
		mutable bool                                             m_DescriptorDirty{ true };
	};

} // namespace cbk::platform::vk
