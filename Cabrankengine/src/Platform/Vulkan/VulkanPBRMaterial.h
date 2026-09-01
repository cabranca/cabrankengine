// #pragma once

// #include <volk/volk.h>

// #include <Cabrankengine/Renderer/Materials/PBRMaterial.h>

// #include "IVulkanRecordable.h"

// namespace cbk::platform::vk {

// 	class VulkanPBRMaterial : public rendering::PBRMaterial {
// 	  public:
// 		VulkanPBRMaterial();
// 		~VulkanPBRMaterial() override;

// 		// Per-class pipeline state cleanup. Called by VulkanRendererAPI::shutdown().
// 		static void destroySharedResources();

// 	  private:
// 		struct PushData {
// 			math::Vector3 albedoColor;
// 			float metalness;
// 			float roughness;
// 		};

// 		static void initSharedResourcesIfNeeded();
// 		void updateDescriptorSet() const;

// 		static bool s_Initialized;
// 		static VkDescriptorSetLayout s_DescriptorSetLayout;
// 		static VkDescriptorPool s_DescriptorPool;
// 		static VkPipelineLayout s_PipelineLayout;
// 		static VkPipeline s_Pipeline;

// 		VkDescriptorSet m_DescriptorSet{ VK_NULL_HANDLE };
// 		mutable bool m_DescriptorDirty{ true };
// 		static inline VkSampleCountFlagBits s_MSAA = VK_SAMPLE_COUNT_1_BIT;
// 	};

// } // namespace cbk::platform::vk
