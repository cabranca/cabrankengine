#pragma once

#include <volk/volk.h>
#include <vma/vma_mem_alloc.h>

namespace cbk::platform::vk {

	class VulkanBuffer {
	  public:
		void init(VkDevice device, VmaAllocator allocator, VkBufferUsageFlagBits usage);
		void shutdown();

        void setData(const void* data, uint32_t size, uint32_t offset);

	  private:
		VkDevice m_Device = VK_NULL_HANDLE;        // NON-OWNING
		VmaAllocator m_Allocator = VK_NULL_HANDLE; // NON-OWNING
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};
} // namespace cbk::platform::vk