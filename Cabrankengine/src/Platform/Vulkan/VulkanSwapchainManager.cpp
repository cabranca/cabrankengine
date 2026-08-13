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
		createSwapchain(VK_NULL_HANDLE);
		createImageViews();
		createRenderFinishedSemaphores();
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
		m_DepthFormat = findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		                                    VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
		VkImageCreateInfo depthImageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = m_DepthFormat,
			.extent{ .width = m_Extent.width, .height = m_Extent.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			// Never read outside the render pass that writes it (LOAD_OP_CLEAR / STORE_OP_DONT_CARE),
			// so TRANSIENT_ATTACHMENT lets the driver keep it tile-local on tilers.
			.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };

		VkImageCreateInfo colorImageCI = depthImageCI;
		colorImageCI.format = m_SelectedFormat.format;
		// Unlike depth, this image's contents are read back by ImGui in a later pass, so it
		// cannot be TRANSIENT — it needs COLOR_ATTACHMENT to render into and SAMPLED to display.
		colorImageCI.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
			VK_CHECK(vmaCreateImage(m_Allocator, &depthImageCI, &allocCI, &m_DepthImages[i], &m_DepthImageAllocations[i], nullptr));
			VkImageViewCreateInfo depthViewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                               .image = m_DepthImages[i],
				                               .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                               .format = m_DepthFormat,
				                               .subresourceRange{
				                                   .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 } };
			VK_CHECK(vkCreateImageView(m_Device, &depthViewCI, nullptr, &m_DepthImageViews[i]));

			VK_CHECK(vmaCreateImage(m_Allocator, &colorImageCI, &allocCI, &m_ColorImages[i], &m_ColorImageAllocations[i], nullptr));
			VkImageViewCreateInfo colorViewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                               .image = m_ColorImages[i],
				                               .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                               .format = m_SelectedFormat.format,
				                               .subresourceRange{
				                                   .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
			VK_CHECK(vkCreateImageView(m_Device, &colorViewCI, nullptr, &m_ColorImageViews[i]));
		}
	}

	void VulkanSwapchainManager::destroyRenderTargets() {
		for (uint32_t i = 0; i < k_MaxFramesInFlight; i++) {
			vkDestroyImageView(m_Device, m_ColorImageViews[i], nullptr);
			vmaDestroyImage(m_Allocator, m_ColorImages[i], m_ColorImageAllocations[i]);

			vkDestroyImageView(m_Device, m_DepthImageViews[i], nullptr);
			vmaDestroyImage(m_Allocator, m_DepthImages[i], m_DepthImageAllocations[i]);
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

	VkImage VulkanSwapchainManager::getDepthImage(uint32_t frameIndex) const {
		return m_DepthImages.at(frameIndex);
	}

	VkImage VulkanSwapchainManager::getColorImage(uint32_t frameIndex) const {
		return m_ColorImages.at(frameIndex);
	}

	VkImageView VulkanSwapchainManager::getSwapchainImageView(uint32_t index) const {
		return m_SwapchainImageViews.at(index);
	}

	VkImageView VulkanSwapchainManager::getDepthImageView(uint32_t frameIndex) const {
		return m_DepthImageViews.at(frameIndex);
	}

	VkImageView VulkanSwapchainManager::getColorImageView(uint32_t frameIndex) const {
		return m_ColorImageViews.at(frameIndex);
	}

	VkSemaphore VulkanSwapchainManager::getSemaphore(uint32_t index) const {
		return m_RenderFinishedSemaphores[index];
	}

	VkExtent2D VulkanSwapchainManager::getExtent() const {
		return m_Extent;
	}
} // namespace cbk::platform::vk
