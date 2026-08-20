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

	struct DescriptorSetsInfo {
		uint32_t FirstSet;
		uint32_t DescriptorSetCount;
		const VkDescriptorSet* DescriptorSets;
	};

	struct PushConstantsInfo {
		VkShaderStageFlags StageFlags;
		uint32_t Offset;
		uint32_t Size;
		const void* Values;
	};

	class VulkanCommands {
	  public:
		static void copyBuffer(VkCommandBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		static void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		static void transitionImageLayout(VkCommandBuffer cb, const std::vector<ImageBarrierInfo>& info);
		static void bindPipeline(VkCommandBuffer cb, VkPipeline pipeline, VkPipelineLayout layout, DescriptorSetsInfo descriptorSets, PushConstantsInfo pushConstants);
	};
} // namespace cbk::platform::vk
