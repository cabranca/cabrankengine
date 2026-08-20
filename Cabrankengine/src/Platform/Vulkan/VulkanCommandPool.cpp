#include <pch.h>
#include "VulkanCommandPool.h"

#include "VkCheck.h"

namespace cbk::platform::vk {

    void VulkanCommandPool::init(VkDevice device, uint32_t familyIndex) {
        m_Device = device;
        VkCommandPoolCreateInfo poolCI {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = familyIndex
		};
		VK_CHECK(vkCreateCommandPool(m_Device, &poolCI, nullptr, &m_Pool));
		CBK_CORE_DEBUG("Vulkan Command Pool created");
    }

    void VulkanCommandPool::shutdown() {
        if (m_Pool != VK_NULL_HANDLE)
			vkDestroyCommandPool(m_Device, m_Pool, nullptr);
    }

    std::vector<VkCommandBuffer> VulkanCommandPool::allocateBuffers(uint32_t count) const {
        std::vector<VkCommandBuffer> buffers(count);
        
        VkCommandBufferAllocateInfo cmdBufferAI {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = m_Pool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = count
		};
		VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdBufferAI, buffers.data()));

        return buffers;
    }
}
