#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Renderer/GraphicsContext.h>

namespace cbk::platform::vk {

	class VulkanDeviceContext : public rendering::GraphicsContext {
	  public:
		VulkanDeviceContext(GLFWwindow* windowHandle);

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
        [[nodiscard]] VkFormat getDepthFormat() const;

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

		static constexpr VkFormat k_ImageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
		VkFormat m_DepthFormat{ VK_FORMAT_UNDEFINED };

		bool createVulkanInstance();
		bool createVulkanDevice();
		bool createAllocator();
		void setDepthFormat();
	};
} // namespace cbk::platform::vk