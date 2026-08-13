#pragma once

#include <volk/volk.h>

#include <vector>

#include "VulkanCommandPool.h"

namespace cbk::platform::vk {

	class VulkanQueue {
	  public:
		void init(VkDevice device, uint32_t familyIndex);
		void shutdown();

		void waitIdle();

		[[nodiscard]] VkQueue getQueue() const;
		[[nodiscard]] uint32_t getQueueFamilyIndex() const;
		[[nodiscard]] std::vector<VkCommandBuffer> allocateCommandBuffers(uint32_t count) const;

		void submitCommands(VkCommandBuffer buffer, VkFence fence, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore,
		                    const std::vector<VkPipelineStageFlags>& waitDstStageMask) const;
		VkResult present(VkSemaphore waitSemaphore, VkSwapchainKHR swapchain, uint32_t imageIndex) const;

	  private:
		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkQueue m_Queue = VK_NULL_HANDLE;
		uint32_t m_FamilyIndex = 0;
		VulkanCommandPool m_Pool;
	};
} // namespace cbk::platform::vk