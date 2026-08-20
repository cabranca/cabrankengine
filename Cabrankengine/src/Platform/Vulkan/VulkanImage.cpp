#include <pch.h>
#include "VulkanImage.h"

#include "VkCheck.h"

namespace cbk::platform::vk {

	void VulkanImage::init(VkDevice device, VmaAllocator allocator, VkFormat format, VkExtent2D extent, uint32_t mipLevels,
	                       VkSampleCountFlagBits msaa, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect) {
		m_Device = device;
		m_Allocator = allocator;
		allocateImage(format, extent, mipLevels, msaa, tiling, usage);
		createView(format, aspect);
	}

	void VulkanImage::destroy() {
		vkDestroyImageView(m_Device, m_View, nullptr);
		vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
	}

	void VulkanImage::allocateImage(VkFormat format, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits msaa,
	                                VkImageTiling tiling, VkImageUsageFlags usage) {
		VkImageCreateInfo imageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent{ .width = extent.width, .height = extent.height, .depth = 1 },
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = msaa,
			.tiling = tiling,
			// Never read outside the render pass that writes it (LOAD_OP_CLEAR / STORE_OP_DONT_CARE),
			// so TRANSIENT_ATTACHMENT lets the driver keep it tile-local on tilers.
			.usage = usage,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };

		VK_CHECK(vmaCreateImage(m_Allocator, &imageCI, &allocCI, &m_Image, &m_Allocation, nullptr));
	}

	void VulkanImage::createView(VkFormat format, VkImageAspectFlags aspect) {
		VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			                               .image = m_Image,
			                               .viewType = VK_IMAGE_VIEW_TYPE_2D,
			                               .format = format,
			                               .subresourceRange{ .aspectMask = aspect, .levelCount = 1, .layerCount = 1 } };
		VK_CHECK(vkCreateImageView(m_Device, &viewCI, nullptr, &m_View));
	}

	VkImage VulkanImage::getImage() const {
		return m_Image;
	}

	VkImageView VulkanImage::getView() const {
		return m_View;
	}
} // namespace cbk::platform::vk
