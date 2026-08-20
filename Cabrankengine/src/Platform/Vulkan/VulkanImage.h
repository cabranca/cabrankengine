#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace cbk::platform::vk {

	class VulkanImage {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkFormat format, VkExtent2D extent, uint32_t mipLevels,
		          VkSampleCountFlagBits msaa, VkImageTiling tiling, VkImageUsageFlags usage, VkImageAspectFlags aspect);
		void destroy();

		[[nodiscard]] VkImage getImage() const;
		[[nodiscard]] VkImageView getView() const;

	  private:
		VkDevice m_Device = VK_NULL_HANDLE;        // NON-OWNING
		VmaAllocator m_Allocator = VK_NULL_HANDLE; // NON-OWNING
		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_View = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;

		void allocateImage(VkFormat format, VkExtent2D extent, uint32_t mipLevels, VkSampleCountFlagBits msaa, VkImageTiling tiling,
		                   VkImageUsageFlags usage);
		void createView(VkFormat format, VkImageAspectFlags aspect);
	};
} // namespace cbk::platform::vk
