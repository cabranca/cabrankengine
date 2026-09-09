#pragma once

#include "VulkanGraphicsPipeline.h"

namespace cbk::platform::vk {

	class VulkanPhongGraphicsPipeline {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
        void shutdown();

		void setSceneData(const rendering::SceneData& sceneData, uint32_t frameIndex);
		void bind(VkCommandBuffer cb, uint32_t frameIndex, const math::Mat4& transform, float shininess);

	  private:
        struct PushData {
			math::Mat4 transform;
			float shininess;
		};

		VulkanGraphicsPipeline m_Pipeline;
	};
} // namespace cbk::platform::vk