#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Renderer/GraphicsContext.h>

namespace cbk::platform::vk {

	class VulkanDeviceContext : public rendering::GraphicsContext {
	  public:
		void init() override;
		void shutdown() override;
		void swapBuffers() override;

		[[nodiscard]] VkInstance getInstance() const;
		[[nodiscard]] VkDevice getLogicalDevice() const;
		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const;
		[[nodiscard]] VkQueue getDeviceQueue() const;
		[[nodiscard]] VmaAllocator getAllocator() const;
        [[nodiscard]] uint32_t getQueueFamily() const;
		[[nodiscard]] VkFormat getImageFormat() const;
		[[nodiscard]] VkColorSpaceKHR getImageColorSpace() const;
        [[nodiscard]] VkFormat getDepthFormat() const;

		void selectSurfaceFormat(VkSurfaceKHR surface);

	  private:
		VkInstance m_Instance;
		VkDevice m_LogicalDevice;
		VkPhysicalDevice m_PhysicalDevice;
		VkQueue m_DeviceQueue;
		VmaAllocator m_Allocator{};
		uint32_t m_QueueFamilyIndex{ 0 };
#ifdef CBK_DEBUG
		VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };
#endif

		VkSurfaceFormatKHR m_SurfaceFormat{};
		VkFormat m_DepthFormat{ VK_FORMAT_UNDEFINED };

		void createVulkanInstance();
		void createVulkanDevice();
		void createAllocator();
		void setDepthFormat();
	};
} // namespace cbk::platform::vk