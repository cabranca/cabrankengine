#include <pch.h>
#include "VulkanCommands.h"

namespace cbk::platform::vk {

	void VulkanCommands::copyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		VkBufferCopy region{ .srcOffset = 0, .dstOffset = 0, .size = size };
		vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &region);
	}

	void VulkanCommands::copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
		VkBufferImageCopy2 region{
			.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
			.pNext = nullptr,
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1 },
			.imageOffset = { .x = 0, .y = 0, .z = 0 },
			.imageExtent = { .width = width, .height = height, .depth = 1 }
		};
		VkCopyBufferToImageInfo2 copyInfo{ .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
			                               .pNext = nullptr,
			                               .srcBuffer = buffer,
			                               .dstImage = image,
			                               .dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                               .regionCount = 1,
			                               .pRegions = &region };
		vkCmdCopyBufferToImage2(cmdBuffer, &copyInfo);
	}

	void VulkanCommands::transitionImageLayout(VkCommandBuffer cb, const std::vector<ImageBarrierInfo>& info) {
		std::vector<VkImageMemoryBarrier2> imageBarriers(info.size());
		for (size_t i = 0; i < info.size(); i++) {
			const auto& barrier = info[i];
			imageBarriers[i] = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
				.pNext = nullptr,
				.srcStageMask = barrier.SrcStageMask,
				.srcAccessMask = barrier.SrcAccessMask,
				.dstStageMask = barrier.DstStageMask,
				.dstAccessMask = barrier.DstAccessMask,
				.oldLayout = barrier.OldLayout,
				.newLayout = barrier.NewLayout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = barrier.Image,
				.subresourceRange = { .aspectMask = barrier.AspectFlags,
										.baseMipLevel = 0,
										.levelCount = barrier.MipLevels,
										.baseArrayLayer = 0,
										.layerCount = 1 } 
			};
		}

		VkDependencyInfo dependencyInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			                             .pNext = nullptr,
			                             .dependencyFlags = 0,
			                             .memoryBarrierCount = 0,
			                             .pMemoryBarriers = nullptr,
			                             .bufferMemoryBarrierCount = 0,
			                             .pBufferMemoryBarriers = nullptr,
			                             .imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size()),
			                             .pImageMemoryBarriers = imageBarriers.data() };

		vkCmdPipelineBarrier2(cb, &dependencyInfo);
	}
} // namespace lab::vk