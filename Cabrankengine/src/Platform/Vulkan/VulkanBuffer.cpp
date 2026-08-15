#include "VulkanBuffer.h"

#include "VkCheck.h"

namespace cbk::platform::vk {

	void VulkanBuffer::init(VkDevice device, VmaAllocator allocator, VkBufferUsageFlagBits usage) {
		m_Device = device;
		m_Allocator = allocator;

        // 1) Per-slot persistently mapped host-visible buffer. Independent allocations
		// so the GPU can read slot N-1 while the CPU writes slot N.
		VkBufferCreateInfo bufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage,
		};
		VmaAllocationCreateInfo allocCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			         VMA_ALLOCATION_CREATE_MAPPED_BIT, // TODO: check these flags to see what they are and if they should be parameters
			.usage = VMA_MEMORY_USAGE_AUTO,
		};
		
		VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferCI, &allocCI, &m_Buffer, &m_Allocation, &m_AllocationInfo));
	}

	void VulkanBuffer::shutdown() {
        vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
    }

    void VulkanBuffer::setData(const void* data, uint32_t size, uint32_t offset) {
        VmaAllocationInfo allocInfo;
		vmaGetAllocationInfo(m_Allocator, m_Allocation, &allocInfo);
		if (!allocInfo.pMappedData) {
			CBK_CORE_ERROR("VulkanBuffer::setData: buffer is not mapped"); // TODO: add isMapped so the user can do this check and return a better error message
			return;
		}
		std::memcpy(static_cast<uint8_t*>(allocInfo.pMappedData) + offset, data, size);
	}
} // namespace cbk::platform::vk