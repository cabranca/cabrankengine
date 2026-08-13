#include <pch.h>
#include "VulkanSwapchainManager.h"

#include <GLFW/glfw3.h>
#include <vulkan/vk_enum_string_helper.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "VulkanDeviceContext.h"
#include "VkCheck.h"

namespace cbk::platform::vk {

	std::string getSurfaceFormatStr(VkSurfaceFormatKHR format) {
		return std::format("{} / {}", string_VkFormat(format.format), string_VkColorSpaceKHR(format.colorSpace));
	}

	const char* getPresentModeStr(VkPresentModeKHR presentMode) {
		return string_VkPresentModeKHR(presentMode);
	}

	void VulkanSwapchainManager::init(const VulkanDeviceContext& ctx) {
		m_Device = ctx.getDevice();
		m_PhysicalDevice = ctx.getPhysicalDevice();
		m_Surface = ctx.getSurface();
		m_QueueFamilyIndex = ctx.getQueueFamily();
		m_Allocator = ctx.getAllocator();
		m_MSAA = ctx.getMSAA();
		createSwapchain(VK_NULL_HANDLE);
		createImageViews();
		createRenderFinishedSemaphores();
		m_DepthFormat = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		                                    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
		createRenderTargets();
	}

	void VulkanSwapchainManager::shutdown() {
		for (const auto& semaphore: m_RenderFinishedSemaphores)
			vkDestroySemaphore(m_Device, semaphore, nullptr);

		destroyRenderTargets();

		for (const auto& view: m_SwapchainImageViews)
			vkDestroyImageView(m_Device, view, nullptr);
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
	}

	uint32_t VulkanSwapchainManager::acquireImage(VkSemaphore presentCompleteSempahore) {
		uint32_t imageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX, presentCompleteSempahore, nullptr, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			// The swapchain was rebuilt and no image/semaphore was acquired; signal the caller to skip
			// this frame rather than draw into a stale index.
			recreateSwapchain();
			return UINT32_MAX;
		}
		// VK_SUBOPTIMAL_KHR still acquired an image and signalled the semaphore, so it has to be drawn and
		// presented; the present below reports it again and recreates then.
		if (result != VK_SUBOPTIMAL_KHR) {
			VK_CHECK(result);
		}
		return imageIndex;
	}

	void VulkanSwapchainManager::recreateSwapchain() {
		vkDeviceWaitIdle(m_Device);
		destroyRenderTargets();
		cleanupSwapchain();
		createImageViews();
		createRenderFinishedSemaphores();
		createRenderTargets();
	}

	void VulkanSwapchainManager::createSwapchain(VkSwapchainKHR oldSwapchain) {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities));

		uint32_t surfaceFormatsCount = 0;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatsCount, nullptr));
		std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &surfaceFormatsCount, surfaceFormats.data()));

		uint32_t presentModesCount = 0;
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModesCount, nullptr));
		std::vector<VkPresentModeKHR> presentModes(presentModesCount);
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModesCount, presentModes.data()));

		m_SelectedFormat = chooseSwapSurfaceFormat(surfaceFormats);
		const auto selectedPresentMode = chooseSwapPresentMode(presentModes);
		m_Extent = chooseSwapExtent(surfaceCapabilities);
		m_MinImageCount = chooseSwapMinImageCount(surfaceCapabilities);

		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		auto queueFamilyIndex = m_QueueFamilyIndex;

		VkSwapchainCreateInfoKHR swapchainCI{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			                                  .surface = m_Surface,
			                                  .minImageCount = m_MinImageCount,
			                                  .imageFormat = m_SelectedFormat.format,
			                                  .imageColorSpace = m_SelectedFormat.colorSpace,
			                                  .imageExtent = m_Extent,
			                                  .imageArrayLayers = 1,
			                                  .imageUsage = usage,
			                                  .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			                                  .queueFamilyIndexCount = 1,
			                                  .pQueueFamilyIndices = &m_QueueFamilyIndex,
			                                  .preTransform = surfaceCapabilities.currentTransform,
			                                  .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			                                  .presentMode = selectedPresentMode,
			                                  .clipped = VK_TRUE,
			                                  .oldSwapchain = oldSwapchain };
		VK_CHECK(vkCreateSwapchainKHR(m_Device, &swapchainCI, nullptr, &m_Swapchain));

		uint32_t imageCount = 0;
		VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr));
		m_SwapchainImages.resize(imageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data()));
	}

	VkSurfaceFormatKHR VulkanSwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		VkSurfaceFormatKHR res = availableFormats[0];
		for (const auto& format: availableFormats) {
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				res = format;
		}
		CBK_CORE_DEBUG("Selected surface format is {}", getSurfaceFormatStr(res));
		return res;
	}

	VkPresentModeKHR VulkanSwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		VkPresentModeKHR res = VK_PRESENT_MODE_FIFO_KHR;
		for (const auto& mode: availablePresentModes) {
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				res = mode;
		}
		CBK_CORE_DEBUG("Selected present mode is {}", getPresentModeStr(res));
		return res;
	}

	VkExtent2D VulkanSwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		const auto& window = Application::get().getWindow();
		return { std::clamp<uint32_t>(window.getWidth(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			     std::clamp<uint32_t>(window.getHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
	}

	uint32_t VulkanSwapchainManager::chooseSwapMinImageCount(const VkSurfaceCapabilitiesKHR& surfaceCapabilities) {
		auto minImageCount = std::max(3U, surfaceCapabilities.minImageCount);
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

	void VulkanSwapchainManager::createImageViews() {
		m_SwapchainImageViews.resize(m_SwapchainImages.size());
		for (size_t i = 0; i < m_SwapchainImages.size(); i++) {
			VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			                          .pNext = nullptr,
			                          .flags = 0,
			                          .image = m_SwapchainImages[i],
			                          .viewType = VK_IMAGE_VIEW_TYPE_2D,
			                          .format = m_SelectedFormat.format,
			                          .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY, },
			                          .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			                                                .baseMipLevel = 0,
			                                                .levelCount = 1,
			                                                .baseArrayLayer = 0,
			                                                .layerCount = 1 } };
			VK_CHECK(vkCreateImageView(m_Device, &viewCI, nullptr, &m_SwapchainImageViews[i]));
		}
	}

	void VulkanSwapchainManager::createRenderFinishedSemaphores() {
		for (const auto& semaphore: m_RenderFinishedSemaphores) {
			vkDestroySemaphore(m_Device, semaphore, nullptr);
		}
		m_RenderFinishedSemaphores.assign(m_SwapchainImages.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0 };

		for (auto& semaphore: m_RenderFinishedSemaphores) {
			VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCI, nullptr, &semaphore));
		}
	}

	void VulkanSwapchainManager::createRenderTargets() {
		for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
			m_ColorAttachments[i].init(m_Device, m_Allocator, m_SelectedFormat.format, m_Extent, 1, m_MSAA, VK_IMAGE_TILING_OPTIMAL,
			                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
			m_ResolveAttachments[i].init(m_Device, m_Allocator, m_SelectedFormat.format, m_Extent, 1, VK_SAMPLE_COUNT_1_BIT,
			                             VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			                             VK_IMAGE_ASPECT_COLOR_BIT);
			m_DepthAttachments[i].init(m_Device, m_Allocator, m_DepthFormat, m_Extent, 1, m_MSAA, VK_IMAGE_TILING_OPTIMAL,
			                           VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			                           VK_IMAGE_ASPECT_DEPTH_BIT);
		}
	}

	void VulkanSwapchainManager::destroyRenderTargets() {
		for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
			m_ColorAttachments[i].destroy();
			m_ResolveAttachments[i].destroy();
			m_DepthAttachments[i].destroy();
		}
	}

	VkFormat VulkanSwapchainManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling,
	                                                     VkFormatFeatureFlags features) {
		for (const auto& format: candidates) {
			VkFormatProperties2 props{ .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2 };
			vkGetPhysicalDeviceFormatProperties2(m_PhysicalDevice, format, &props);
			if (((tiling == VK_IMAGE_TILING_LINEAR) && ((props.formatProperties.linearTilingFeatures & features) == features)) ||
			    ((tiling == VK_IMAGE_TILING_OPTIMAL) && ((props.formatProperties.optimalTilingFeatures & features) == features)))
				return format;
		}
		CBK_CORE_FATAL("Failed to find supported format!");
		return VK_FORMAT_MAX_ENUM;
	}

	void VulkanSwapchainManager::cleanupSwapchain() {
		for (const auto& view: m_SwapchainImageViews) {
			vkDestroyImageView(m_Device, view, nullptr);
		}
		m_SwapchainImageViews.clear();
		VkSwapchainKHR oldSwapchain = m_Swapchain;
		createSwapchain(oldSwapchain);
		vkDestroySwapchainKHR(m_Device, oldSwapchain, nullptr);
	}

	uint32_t VulkanSwapchainManager::getMinImageCount() const {
		return m_MinImageCount;
	}

	size_t VulkanSwapchainManager::getSwapchainImagesSize() const {
		return m_SwapchainImages.size();
	}

	VkSwapchainKHR VulkanSwapchainManager::getSwapchain() {
		return m_Swapchain;
	}

	VkImage VulkanSwapchainManager::getSwapchainImage(uint32_t index) const {
		return m_SwapchainImages.at(index);
	}

	VkImage VulkanSwapchainManager::getColorImage(uint32_t frameIndex) const {
		return m_ColorAttachments[frameIndex].getImage();
	}

	VkImage VulkanSwapchainManager::getResolveImage(uint32_t frameIndex) const {
		return m_ResolveAttachments[frameIndex].getImage();
	}

	VkImage VulkanSwapchainManager::getDepthImage(uint32_t frameIndex) const {
		return m_DepthAttachments[frameIndex].getImage();
	}

	VkImageView VulkanSwapchainManager::getSwapchainImageView(uint32_t index) const {
		return m_SwapchainImageViews.at(index);
	}

	VkImageView VulkanSwapchainManager::getColorImageView(uint32_t frameIndex) const {
		return m_ColorAttachments[frameIndex].getView();
	}

	VkImageView VulkanSwapchainManager::getResolveImageView(uint32_t frameIndex) const {
		return m_ResolveAttachments[frameIndex].getView();
	}

	VkImageView VulkanSwapchainManager::getDepthImageView(uint32_t frameIndex) const {
		return m_DepthAttachments[frameIndex].getView();
	}

	VkSemaphore VulkanSwapchainManager::getSemaphore(uint32_t index) const {
		return m_RenderFinishedSemaphores[index];
	}

	VkExtent2D VulkanSwapchainManager::getExtent() const {
		return m_Extent;
	}
} // namespace cbk::platform::vk
