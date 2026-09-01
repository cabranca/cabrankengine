// #pragma once

// #include <array>

// #include <volk/volk.h>

// #include <Cabrankengine/Renderer/Materials/Texture2DMaterial.h>
// #include <Cabrankengine/Renderer/Texture.h>

// #include "IVulkanRecordable.h"

// namespace cbk::platform::vk {

// 	// Material backing the Renderer2D batch quad pipeline. Owns a 32-sampler
// 	// descriptor set at set index 1; the per-instance slots are populated by the
// 	// batcher via setTextureSlot before flush. No push constants — vertex positions
// 	// are already in world space when the batcher submits them.
// 	class VulkanTexture2DMaterial : public rendering::Texture2DMaterial, public IVulkanRecordable {
// 	  public:
// 		static constexpr uint32_t k_MaxTextureSlots = 32;

// 		VulkanTexture2DMaterial();
// 		~VulkanTexture2DMaterial() override;

// 		// Slot 0 is conventionally the white texture used by un-textured quads.
// 		// Callers must populate slot 0 at least once before the first flush.
// 		void setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) override;

// 		// View-projection comes from the global scene UBO bound by the renderer.
// 		void setViewProjection(const math::Mat4& /*viewProjection*/) override {}

// 		// IVulkanRecordable
// 		void record(VkCommandBuffer cb, const math::Mat4& transform) const override;

// 		// Per-class pipeline state cleanup. Called by VulkanRendererAPI::shutdown().
// 		static void destroySharedResources();

// 	  private:
// 		static void initSharedResourcesIfNeeded();
// 		void updateDescriptorSet() const;

// 		static bool s_Initialized;
// 		static VkDescriptorSetLayout s_DescriptorSetLayout;
// 		static VkDescriptorPool s_DescriptorPool;
// 		static VkPipelineLayout s_PipelineLayout;
// 		static VkPipeline s_Pipeline;

// 		std::array<Ref<rendering::Texture2D>, k_MaxTextureSlots> m_TextureSlots{};
// 		VkDescriptorSet m_DescriptorSet{ VK_NULL_HANDLE };
// 		mutable bool m_DescriptorDirty{ true };
// 		static inline VkSampleCountFlagBits s_MSAA = VK_SAMPLE_COUNT_1_BIT;
// 	};

// } // namespace cbk::platform::vk
