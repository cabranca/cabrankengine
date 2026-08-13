#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <GLFW/glfw3.h>

#include <Cabrankengine/Renderer/GraphicsContext.h>

#include "VulkanInstance.h"
#include "VulkanQueue.h"

namespace cbk::platform::vk {

	class VulkanDeviceContext : public rendering::GraphicsContext {
	  public:
		void init(const Window& window) override;
		void shutdown() override;

		void waitIdle();
		void queueWaitIdle();
		void selectSurfaceFormat(VkSurfaceKHR surface);

		[[nodiscard]] VkInstance getInstance() const;
		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const;
		[[nodiscard]] VkDevice getDevice() const;
		[[nodiscard]] const VulkanQueue& getQueue() const;
		[[nodiscard]] VmaAllocator getAllocator() const;
		[[nodiscard]] uint32_t getQueueFamily() const;
		[[nodiscard]] VkFormat getImageFormat() const;
		[[nodiscard]] VkColorSpaceKHR getImageColorSpace() const;
		[[nodiscard]] VkFormat getDepthFormat() const;
		[[nodiscard]] VkSurfaceKHR getSurface() const;

	  private:
		VulkanInstance m_Instance;
		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;
		uint32_t m_QueueFamilyIndex{ 0 };
		VulkanQueue m_Queue;
		VmaAllocator m_Allocator{};

		VkPhysicalDeviceMemoryProperties m_MemoryProperties;

		constexpr static VkSampleCountFlagBits k_MaxMSAA = VK_SAMPLE_COUNT_8_BIT;
		VkSampleCountFlagBits m_MSAASamples = VK_SAMPLE_COUNT_1_BIT;

		VkSurfaceFormatKHR m_SurfaceFormat{};
		VkFormat m_DepthFormat{ VK_FORMAT_UNDEFINED };

		void pickPhysicalDevice();
		[[nodiscard]] VkSampleCountFlagBits getMaxUsableSampleCount();
		void createVulkanDevice();
		void createAllocator();
		void setDepthFormat();
	};
} // namespace cbk::platform::vk