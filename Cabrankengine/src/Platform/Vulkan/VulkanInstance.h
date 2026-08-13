#pragma once

#include <volk/volk.h>

struct GLFWwindow;

namespace cbk::platform::vk {

	class VulkanInstance {
	  public:
		void init(GLFWwindow* window);
		void shutdown();

		[[nodiscard]] VkInstance getInstance() const;
		[[nodiscard]] VkSurfaceKHR getSurface() const;

	  private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		GLFWwindow* m_WindowHandle = nullptr; // NON OWNING

#ifdef CBK_DEBUG
		VkDebugUtilsMessengerEXT m_DebugMessenger{ VK_NULL_HANDLE };
#endif

		void createInstance();
		void createDebugMessenger();
		void createSurface();
	};
} // namespace cbk::platform::vk