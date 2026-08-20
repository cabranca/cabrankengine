#include <pch.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include "VulkanInstance.h"

#include <GLFW/glfw3.h>

#include "VkCheck.h"

namespace cbk::platform::vk {

	std::vector<VkLayerProperties> getAvailableLayers() {
		uint32_t layerPropCount = 0;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerPropCount, nullptr));
		std::vector<VkLayerProperties> availableLayers(layerPropCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerPropCount, availableLayers.data()));

		return availableLayers;
	}

	std::vector<VkExtensionProperties> getAvailableExtensions() {
		uint32_t extensionsCount = 0;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr));
		std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data()));

		return availableExtensions;
	}

#ifdef CBK_DEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	                                                          VkDebugUtilsMessageTypeFlagsEXT /*type*/,
	                                                          const VkDebugUtilsMessengerCallbackDataEXT* data, void* /*userData*/) {
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			CBK_CORE_ERROR("[Vulkan] {}", data->pMessage);
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			CBK_CORE_WARN("[Vulkan] {}", data->pMessage);
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			CBK_CORE_INFO("[Vulkan] {}", data->pMessage);
		else
			CBK_CORE_TRACE("[Vulkan] {}", data->pMessage);
		return VK_FALSE;
	}

	static VkDebugUtilsMessengerCreateInfoEXT makeDebugMessengerCI() {
		return VkDebugUtilsMessengerCreateInfoEXT{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vulkanDebugCallback,
		};
	}
#endif

	void VulkanInstance::init(GLFWwindow* window) {
		m_WindowHandle = window;
		createInstance();
		createSurface();

#ifdef CBK_DEBUG
		createDebugMessenger();
#endif
	}

	void VulkanInstance::shutdown() {
#ifdef CBK_DEBUG
		if (m_DebugMessenger != VK_NULL_HANDLE)
			vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
#endif
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanInstance::createInstance() {
		if (glfwVulkanSupported() != GLFW_TRUE) {
			CBK_CORE_FATAL("Vulkan not supported");
			std::abort();
		}
		VK_CHECK(volkInitialize());

		VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			                       .pApplicationName = "Cabrankengine",
			                       .apiVersion = VK_API_VERSION_1_4 };

		const auto availableLayers = getAvailableLayers();
		const auto availableExtensions = getAvailableExtensions();

		std::vector<const char*> layers;
		VkInstanceCreateFlags flags = 0;
#ifdef CBK_DEBUG
		bool debugUtilsAvailable = false;
#endif

		uint32_t glfwExtCount = 0;
		char const* const* glfwExts{ glfwGetRequiredInstanceExtensions(&glfwExtCount) };
		std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

		for (const auto& extension: availableExtensions) {
			if (std::strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
				extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
		}

		for (const auto& layer: availableLayers) {
			if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0) {
				layers.emplace_back("VK_LAYER_KHRONOS_validation");
			}
		}

#ifdef CBK_DEBUG
		debugUtilsAvailable = true;
		auto debugMessengerCI = makeDebugMessengerCI();
#endif

		VkInstanceCreateInfo instanceCI{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef CBK_DEBUG
			.pNext = debugUtilsAvailable ? &debugMessengerCI : nullptr,
#endif
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(layers.size()),
			.ppEnabledLayerNames = layers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};
		VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
	}

	void VulkanInstance::createDebugMessenger() {
#ifdef CBK_DEBUG
		auto messengerCI = makeDebugMessengerCI();
		VkResult vkResult = vkCreateDebugUtilsMessengerEXT(m_Instance, &messengerCI, nullptr, &m_DebugMessenger);
		if (vkResult != VK_SUCCESS)
			CBK_CORE_WARN("vkCreateDebugUtilsMessengerEXT failed ({}); validation messages will not be reported",
			              static_cast<int>(vkResult));
#endif
	}

	void VulkanInstance::createSurface() {
		VK_CHECK(glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface));
		CBK_CORE_ASSERT(m_Surface != VK_NULL_HANDLE, "VulkanSwapchainManager: surface is null after glfwCreateWindowSurface");
	}

	VkInstance VulkanInstance::getInstance() const {
		return m_Instance;
	}

	VkSurfaceKHR VulkanInstance::getSurface() const {
		return m_Surface;
	}
} // namespace cbk::platform::vk
