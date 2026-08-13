#include <pch.h>

#include "VulkanDeviceContext.h"
#include "VkCheck.h"

#include <GLFW/glfw3.h>

namespace cbk::platform::vk {

	void VulkanDeviceContext::init(const Window& window) {
		m_Instance.init(static_cast<GLFWwindow*>(window.getNativeWindow()));
		pickPhysicalDevice();
		selectSurfaceFormat(m_Instance.getSurface());
		createVulkanDevice();
		m_Queue.init(m_Device, m_QueueFamilyIndex);
		createAllocator();
		setDepthFormat();
	}

	void VulkanDeviceContext::shutdown() {
		vmaDestroyAllocator(m_Allocator);
		vkDestroyDevice(m_Device, nullptr);
		// DESTROY SURFACE
	}

	void VulkanDeviceContext::waitIdle() {
		vkDeviceWaitIdle(m_Device);
	}

	void VulkanDeviceContext::queueWaitIdle() {
		m_Queue.waitIdle();
	}

	void VulkanDeviceContext::selectSurfaceFormat(VkSurfaceKHR surface) {
		uint32_t count = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &count, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, surface, &count, formats.data());

		constexpr VkFormat preferred[] = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB };
		for (VkFormat pref: preferred) {
			for (auto& f: formats) {
				if (f.format == pref && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					m_SurfaceFormat = f;
					return;
				}
			}
		}
		m_SurfaceFormat = formats[0];
		CBK_CORE_WARN("VulkanDeviceContext: preferred sRGB surface format unavailable, falling back to format {}",
		              static_cast<int>(m_SurfaceFormat.format));
	}

	void VulkanDeviceContext::pickPhysicalDevice() {
		const VkInstance instance = m_Instance.getInstance();

		uint32_t deviceCount = 0;
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
		std::vector<VkPhysicalDevice> devices(deviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

		for (const auto& device: devices) {
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(device, &props);

			if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && props.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
				CBK_CORE_DEBUG("{}: not a discrete or integrated GPU", props.deviceName);
				continue;
			}
			if (props.apiVersion < VK_API_VERSION_1_4) {
				CBK_CORE_DEBUG("{}: VK API version is less than 1.4", props.deviceName);
				continue;
			}

			uint32_t qFamilyPropCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyPropCount, nullptr);
			std::vector<VkQueueFamilyProperties> qFamilyProps(qFamilyPropCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyPropCount, qFamilyProps.data());

			bool supportsGraphics = false;
			for (uint32_t i = 0; i < qFamilyPropCount; i++) {
				const auto& qFamily = qFamilyProps[i];
				VkBool32 supportsSurface = VK_FALSE;
				VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Instance.getSurface(), &supportsSurface));
				if ((qFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && supportsSurface == VK_TRUE) {
					supportsGraphics = true;
					m_QueueFamilyIndex = i;
					break;
				}
			}

			if (!supportsGraphics) {
				CBK_CORE_DEBUG("{}: no graphics queue", props.deviceName);
				continue;
			}

			if (glfwGetPhysicalDevicePresentationSupport(instance, device, m_QueueFamilyIndex) != GLFW_TRUE) {
				CBK_CORE_DEBUG("{}: no presentation support on queue {}", props.deviceName, m_QueueFamilyIndex);
				continue;
			}

			std::vector<const char*> requiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
			uint32_t extensionPropCount = 0;
			VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionPropCount, nullptr));
			std::vector<VkExtensionProperties> deviceExtensions(extensionPropCount);
			VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionPropCount, deviceExtensions.data()));

			bool supportsExtensions = true;
			for (const auto& required: requiredDeviceExtensions) {
				bool supportsExtension = false;
				for (const auto& available: deviceExtensions) {
					if (std::strcmp(required, available.extensionName) == 0)
						supportsExtension = true;
				}
				if (!supportsExtension) {
					CBK_CORE_DEBUG("{}: missing extension {}", props.deviceName, required);
					supportsExtensions = false;
				}
			}
			if (!supportsExtensions)
				continue;

			// Chained so one query fills all three; each struct is zero-initialized because a driver
			// leaves any sType it does not recognize untouched.
			VkPhysicalDeviceVulkan13Features vk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
			VkPhysicalDeviceVulkan11Features vk11Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
				                                           .pNext = &vk13Features };
			VkPhysicalDeviceFeatures2 features2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &vk11Features };
			vkGetPhysicalDeviceFeatures2(device, &features2);

			if (features2.features.tessellationShader != VK_TRUE) {
				CBK_CORE_DEBUG("{}: no tessellation shader support", props.deviceName);
				continue;
			}
			if (vk11Features.shaderDrawParameters != VK_TRUE) {
				CBK_CORE_DEBUG("{}: no shader draw parameters support", props.deviceName);
				continue;
			}
			if (vk13Features.dynamicRendering != VK_TRUE) {
				CBK_CORE_DEBUG("{}: no dynamic rendering support", props.deviceName);
				continue;
			}

			m_PhysicalDevice = device;
			CBK_CORE_INFO("Physical Device: {}", props.deviceName);
			break;
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE) {
			CBK_CORE_ERROR("No physical device met the requirements!");
		} else
			m_MSAASamples = getMaxUsableSampleCount();
	}

	VkSampleCountFlagBits VulkanDeviceContext::getMaxUsableSampleCount() {
		VkPhysicalDeviceProperties prop;
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &prop);
		// Each sample count is a single bit, so every count up to and including the cap is (k_MaxMSAA << 1) - 1.
		constexpr VkSampleCountFlags allowed = (static_cast<VkSampleCountFlags>(k_MaxMSAA) << 1) - 1;
		VkSampleCountFlags counts = prop.limits.framebufferColorSampleCounts & prop.limits.framebufferDepthSampleCounts & allowed;
		if (counts & VK_SAMPLE_COUNT_64_BIT)
			return VK_SAMPLE_COUNT_64_BIT;
		if (counts & VK_SAMPLE_COUNT_32_BIT)
			return VK_SAMPLE_COUNT_32_BIT;
		if (counts & VK_SAMPLE_COUNT_16_BIT)
			return VK_SAMPLE_COUNT_16_BIT;
		if (counts & VK_SAMPLE_COUNT_8_BIT)
			return VK_SAMPLE_COUNT_8_BIT;
		if (counts & VK_SAMPLE_COUNT_4_BIT)
			return VK_SAMPLE_COUNT_4_BIT;
		if (counts & VK_SAMPLE_COUNT_2_BIT)
			return VK_SAMPLE_COUNT_2_BIT;

		return VK_SAMPLE_COUNT_1_BIT;
	}

	void VulkanDeviceContext::createVulkanDevice() {
		const VkInstance instance = m_Instance.getInstance();

		const float qfPriorities{ 0.5f };
		VkDeviceQueueCreateInfo queueCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			                             .queueFamilyIndex = m_QueueFamilyIndex,
			                             .queueCount = 1,
			                             .pQueuePriorities = &qfPriorities };

		VkPhysicalDeviceVulkan13Features vk13Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
			                                           .synchronization2 = VK_TRUE,
			                                           .dynamicRendering = VK_TRUE };
		VkPhysicalDeviceVulkan12Features vk12Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
			                                           .pNext = &vk13Features,
			                                           .descriptorIndexing = true,
			                                           .shaderSampledImageArrayNonUniformIndexing = true,
			                                           .descriptorBindingVariableDescriptorCount = true,
			                                           .runtimeDescriptorArray = true,
			                                           .bufferDeviceAddress = true };
		VkPhysicalDeviceVulkan11Features vk11Features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			                                           .pNext = &vk12Features,
			                                           .shaderDrawParameters = VK_TRUE };
		VkPhysicalDeviceFeatures2 features2{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			                                 .pNext = &vk11Features,
			                                 .features{ .sampleRateShading = VK_TRUE, .samplerAnisotropy = VK_TRUE } };

		const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		VkDeviceCreateInfo deviceCI{ .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			                         .pNext = &features2,
			                         .queueCreateInfoCount = 1,
			                         .pQueueCreateInfos = &queueCI,
			                         .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			                         .ppEnabledExtensionNames = deviceExtensions.data(),
			                         .pEnabledFeatures = nullptr };
		VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCI, nullptr, &m_Device));
		volkLoadDevice(m_Device);
	}

	void VulkanDeviceContext::createAllocator() {
		VmaVulkanFunctions vkFunctions{ .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
			                            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
			                            .vkCreateImage = vkCreateImage };
		VmaAllocatorCreateInfo allocatorCI{ .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
			                                .physicalDevice = m_PhysicalDevice,
			                                .device = m_Device,
			                                .pVulkanFunctions = &vkFunctions,
			                                .instance = m_Instance.getInstance() };
		VK_CHECK(vmaCreateAllocator(&allocatorCI, &m_Allocator));
	}

	void VulkanDeviceContext::setDepthFormat() {
		// Must match VulkanSwapchainManager's own depth-format candidate list exactly — both
		// query the same physical device for the same tiling/feature, so an identical list
		// picks the identical format. Pipelines are built against this format; the swapchain
		// manager's depth images are created against its own. A mismatch is invalid per the
		// VK_KHR_dynamic_rendering spec: VkRenderingInfo's depth image view format must equal
		// VkPipelineRenderingCreateInfo::depthAttachmentFormat.
		std::vector<VkFormat> depthFormatList{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
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

	VkInstance VulkanDeviceContext::getInstance() const {
		return m_Instance.getInstance();
	}

	VkPhysicalDevice VulkanDeviceContext::getPhysicalDevice() const {
		return m_PhysicalDevice;
	}

	VkDevice VulkanDeviceContext::getDevice() const {
		return m_Device;
	}

	const VulkanQueue& VulkanDeviceContext::getQueue() const {
		return m_Queue;
	}

	VmaAllocator VulkanDeviceContext::getAllocator() const {
		return m_Allocator;
	}

	uint32_t VulkanDeviceContext::getQueueFamily() const {
		return m_QueueFamilyIndex;
	}

	VkFormat VulkanDeviceContext::getImageFormat() const {
		return m_SurfaceFormat.format;
	}

	VkColorSpaceKHR VulkanDeviceContext::getImageColorSpace() const {
		return m_SurfaceFormat.colorSpace;
	}

	VkFormat VulkanDeviceContext::getDepthFormat() const {
		return m_DepthFormat;
	}

	VkSurfaceKHR VulkanDeviceContext::getSurface() const {
		return m_Instance.getSurface();
	}

	VkSampleCountFlagBits VulkanDeviceContext::getMSAA() const {
		return m_MSAASamples;
	}
} // namespace cbk::platform::vk
