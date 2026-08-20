#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace cbk::platform::vk {

	class VulkanBuffer {
	  public:
		void init(VkDevice device, VmaAllocator allocator, uint32_t size, VkBufferUsageFlags usage);
		void shutdown();

        void setData(const void* data, uint32_t size, uint32_t offset);

		[[nodiscard]] VkBuffer getBuffer() const;

	  private:
		VkDevice m_Device = VK_NULL_HANDLE;        // NON-OWNING
		VmaAllocator m_Allocator = VK_NULL_HANDLE; // NON-OWNING
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VmaAllocationInfo m_AllocInfo;
	};
} // namespace cbk::platform::vk
