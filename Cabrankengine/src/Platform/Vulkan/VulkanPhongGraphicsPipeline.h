#pragma once

#include "VulkanGraphicsPipeline.h"

namespace cbk::platform::vk {

	class VulkanPhongGraphicsPipeline {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, VkSampleCountFlagBits sampleCount);
		void shutdown();

		void bind(VkCommandBuffer cb, uint32_t frameIndex, VkDescriptorSet materialSet, const math::Mat4& transform, float shininess);

		[[nodiscard]] VkDescriptorSet allocateDescriptorSet();

	  private:
		// Mirrors PushConstants in Phong.slang.
		struct PushData {
			math::Mat4 transform; // bytes  0..63
			float shininess;      // bytes 64..67
		};
		static_assert(sizeof(PushData) == 68, "PushData must match the push-constant block of Phong.slang");

		VulkanGraphicsPipeline m_Pipeline;
	};
} // namespace cbk::platform::vk