#include <pch.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define VMA_IMPLEMENTATION
#include "VulkanDeviceContext.h"
#include "VkCheck.h"

#include <GLFW/glfw3.h>

namespace cbk::platform::vk {

	void VulkanDeviceContext::init() {
		if (glfwVulkanSupported() != GLFW_TRUE) {
			CBK_CORE_FATAL("Vulkan not supported");
			std::abort();
		}
		VK_CHECK(volkInitialize());
		createVulkanInstance();
		createVulkanDevice();
		createAllocator();
		setDepthFormat();
	}

	void VulkanDeviceContext::shutdown() {
		vmaDestroyAllocator(m_Allocator);
		vkDestroyDevice(m_LogicalDevice, nullptr);
#ifdef CBK_DEBUG
		if (m_DebugMessenger != VK_NULL_HANDLE)
			vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
#endif
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanDeviceContext::swapBuffers() {}

    VkInstance VulkanDeviceContext::getInstance() const {
        return m_Instance;
    }

    VkDevice VulkanDeviceContext::getLogicalDevice() const {
        return m_LogicalDevice;
    }

    VkPhysicalDevice VulkanDeviceContext::getPhysicalDevice() const {
        return m_PhysicalDevice;
    }

	VkQueue VulkanDeviceContext::getDeviceQueue() const {
		return m_DeviceQueue;
	}

	VmaAllocator VulkanDeviceContext::getAllocator() const {
        return m_Allocator;
    }

	uint32_t VulkanDeviceContext::getQueueFamily() const {
		return m_QueueFamilyIndex;
	}

	VkFormat VulkanDeviceContext::getImageFormat() const {
		return k_ImageFormat;
	}

	VkFormat VulkanDeviceContext::getDepthFormat() const {
		return m_DepthFormat;
	}

#ifdef CBK_DEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	                                                         VkDebugUtilsMessageTypeFlagsEXT /*type*/,
	                                                         const VkDebugUtilsMessengerCallbackDataEXT* data,
	                                                         void* /*userData*/) {
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
			.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			               | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vulkanDebugCallback,
		};
	}
#endif

    void VulkanDeviceContext::createVulkanInstance() {
		VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			                       .pApplicationName = "Cabrankengine",
			                       .apiVersion = VK_API_VERSION_1_3 };
		uint32_t glfwExtCount = 0;
		char const* const* glfwExts{ glfwGetRequiredInstanceExtensions(&glfwExtCount) };
		std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);
#ifdef CBK_DEBUG
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		const char* validationLayers[] = { "VK_LAYER_KHRONOS_validation" };
		auto debugMessengerCI = makeDebugMessengerCI();
#endif
		VkInstanceCreateInfo instanceCI{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef CBK_DEBUG
			.pNext = &debugMessengerCI, // catches errors during vkCreateInstance/vkDestroyInstance themselves
#endif
			.pApplicationInfo = &appInfo,
#ifdef CBK_DEBUG
			.enabledLayerCount = 1,
			.ppEnabledLayerNames = validationLayers,
#endif
			.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data(),
		};
		VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
#ifdef CBK_DEBUG
		auto messengerCI = makeDebugMessengerCI();
		VkResult vkResult = vkCreateDebugUtilsMessengerEXT(m_Instance, &messengerCI, nullptr, &m_DebugMessenger);
		if (vkResult != VK_SUCCESS)
			CBK_CORE_WARN("vkCreateDebugUtilsMessengerEXT failed ({}); validation messages will not be reported", static_cast<int>(vkResult));
#endif
	}

	void VulkanDeviceContext::createVulkanDevice() {
        std::vector<VkPhysicalDevice> devices;
		uint32_t deviceCount{ 0 };
        uint32_t deviceIndex{ 0 };
		VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr));
		devices.resize(deviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data()));
        m_PhysicalDevice = devices[deviceIndex];

		VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProperties);
        CBK_CORE_INFO("Vulkan Info:");
		CBK_CORE_INFO("  Device: {0}", deviceProperties.properties.deviceName);

		uint32_t queueFamilyCount{ 0 };
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());
		for (size_t i = 0; i < queueFamilies.size(); i++) {
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				m_QueueFamilyIndex = i;
				break;
			}
		}
		if (glfwGetPhysicalDevicePresentationSupport(m_Instance, m_PhysicalDevice, m_QueueFamilyIndex) != GLFW_TRUE) {
			CBK_CORE_FATAL("VulkanContext: Selected queue family cannot present images");
			std::abort();
		}
		const float qfPriorities{ 1.f };
		VkDeviceQueueCreateInfo queueCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			                             .queueFamilyIndex = m_QueueFamilyIndex,
			                             .queueCount = 1,
			                             .pQueuePriorities = &qfPriorities };
		// Slang emits SPIR-V referencing the DrawParameters capability for vertex shaders,
		// even when the shader doesn't read gl_BaseInstance/etc. directly — enable it
		// at the device level so vkCreateShaderModule accepts the SPIR-V.
		VkPhysicalDeviceVulkan11Features enabledVk11Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			                                                  .shaderDrawParameters = VK_TRUE };
		VkPhysicalDeviceVulkan12Features enabledVk12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			                                                  .pNext = &enabledVk11Features,
			                                                  .descriptorIndexing = true,
			                                                  .shaderSampledImageArrayNonUniformIndexing = true,
			                                                  .descriptorBindingVariableDescriptorCount = true,
			                                                  .runtimeDescriptorArray = true,
			                                                  .bufferDeviceAddress = true };
		VkPhysicalDeviceVulkan13Features enabledVk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			                                                  .pNext = &enabledVk12Features,
			                                                  .synchronization2 = true,
			                                                  .dynamicRendering = true };
		VkPhysicalDeviceFeatures enabledVk10Features{ .samplerAnisotropy = VK_TRUE };
		const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			                         .pNext = &enabledVk13Features,
			                         .queueCreateInfoCount = 1,
			                         .pQueueCreateInfos = &queueCI,
			                         .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			                         .ppEnabledExtensionNames = deviceExtensions.data(),
			                         .pEnabledFeatures = &enabledVk10Features };
		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_LogicalDevice));
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndex, 0, &m_DeviceQueue);
	}

	void VulkanDeviceContext::createAllocator() {
		VmaVulkanFunctions vkFunctions{ .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
			                            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
			                            .vkCreateImage = vkCreateImage };
		VmaAllocatorCreateInfo allocatorCI{ .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			                                .physicalDevice = m_PhysicalDevice,
			                                .device = m_LogicalDevice,
			                                .pVulkanFunctions = &vkFunctions,
			                                .instance = m_Instance };
		VK_CHECK(vmaCreateAllocator(&allocatorCI, &m_Allocator));
	}

	void VulkanDeviceContext::setDepthFormat() {
		std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
		for (VkFormat& format: depthFormatList) {
			VkFormatProperties2 formatProperties{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
			vkGetPhysicalDeviceFormatProperties2(m_PhysicalDevice, format, &formatProperties);
			if (formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
				m_DepthFormat = format;
				break;
			}
		}
		CBK_CORE_ASSERT(m_DepthFormat != VK_FORMAT_UNDEFINED, "Depth Format is undefined");
	}
}
