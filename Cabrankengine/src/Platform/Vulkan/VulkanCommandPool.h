#pragma once

#include <volk/volk.h>

#include <vector>

namespace cbk::platform::vk {

	class VulkanCommandPool {
	  public:
		void init(VkDevice device, uint32_t familyIndex);
		void shutdown();

		[[nodiscard]] std::vector<VkCommandBuffer> allocateBuffers(uint32_t count) const;

	  private:
		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VkCommandPool m_Pool = VK_NULL_HANDLE;
	};
} // namespace cbk::platform::vk
