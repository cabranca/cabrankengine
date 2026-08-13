#include <pch.h>
#include "VulkanQueue.h"

#include "VkCheck.h"

namespace cbk::platform::vk {

    void VulkanQueue::init(VkDevice device, uint32_t familyIndex) {
        vkGetDeviceQueue(device, familyIndex, 0, &m_Queue);
        m_Pool.init(device, familyIndex);
    }

    void VulkanQueue::shutdown() {
        m_Pool.shutdown();
    }

    void VulkanQueue::waitIdle() {
        vkQueueWaitIdle(m_Queue);
    }

    VkQueue VulkanQueue::getQueue() const {
        return m_Queue;
    }

    std::vector<VkCommandBuffer> VulkanQueue::allocateCommandBuffers(uint32_t count) const {
		return m_Pool.allocateBuffers(count);
	}

	void VulkanQueue::submitCommands(VkCommandBuffer buffer, VkFence fence, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore,
	                                 const std::vector<VkPipelineStageFlags>& waitDstStageMask) const {
		VkSemaphoreSubmitInfo waitInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			                            .semaphore = waitSemaphore,
			                            .stageMask = waitDstStageMask.empty()
			                                             ? VK_PIPELINE_STAGE_2_NONE
			                                             : static_cast<VkPipelineStageFlags2>(waitDstStageMask.front()) };
		VkSemaphoreSubmitInfo signalInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
			                              .semaphore = signalSemaphore,
			                              .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkCommandBufferSubmitInfo cmdInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = buffer };

		VkSubmitInfo2 submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			                      .waitSemaphoreInfoCount = 1,
			                      .pWaitSemaphoreInfos = &waitInfo,
			                      .commandBufferInfoCount = 1,
			                      .pCommandBufferInfos = &cmdInfo,
			                      .signalSemaphoreInfoCount = 1,
			                      .pSignalSemaphoreInfos = &signalInfo };
		VK_CHECK(vkQueueSubmit2(m_Queue, 1, &submitInfo, fence));
	}

	VkResult VulkanQueue::present(VkSemaphore waitSemaphore, VkSwapchainKHR swapchain, uint32_t imageIndex) const {
		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &waitSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &swapchain,
			.pImageIndices = &imageIndex,
			.pResults = nullptr
		};
		return vkQueuePresentKHR(m_Queue, &presentInfo);
	}
}