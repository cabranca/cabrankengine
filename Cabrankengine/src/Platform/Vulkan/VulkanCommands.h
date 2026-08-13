#pragma once

#include <volk/volk.h>

#include <vector>

namespace cbk::platform::vk {

	struct ImageBarrierInfo {
		VkImage Image = VK_NULL_HANDLE;
		VkImageLayout OldLayout;
		VkImageLayout NewLayout;
		VkAccessFlags2 SrcAccessMask;
		VkAccessFlags2 DstAccessMask;
		VkPipelineStageFlags2 SrcStageMask;
		VkPipelineStageFlags2 DstStageMask;
		VkImageAspectFlags AspectFlags;
		uint32_t MipLevels = 1;
	};

	class VulkanCommands {
	  public:
		static void copyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		static void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		static void transitionImageLayout(VkCommandBuffer cb, const std::vector<ImageBarrierInfo>& info);
	};
} // namespace cbk::platform::vk