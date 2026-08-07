#include <pch.h>
#include "VulkanSwapchainManager.h"
#include "VkCheck.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "VulkanDeviceContext.h"

#include <GLFW/glfw3.h>

namespace cbk::platform::vk {

	void VulkanSwapchainManager::init(VulkanDeviceContext* ctx) {
		m_Instance = ctx->getInstance();
		m_Device = ctx->getLogicalDevice();
		createSwapchain(ctx);
		getSwapchainImages(ctx);
		createDepthAttachment(ctx);
	}

	void VulkanSwapchainManager::shutdown() {
		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		for (size_t i = 0; i < m_DepthImages.size(); i++) {
			vkDestroyImageView(ctx->getLogicalDevice(), m_DepthImageViews[i], nullptr);
			vmaDestroyImage(ctx->getAllocator(), m_DepthImages[i], m_DepthImageAllocations[i]);
		}
		for (auto i = 0; i < m_SwapchainImageViews.size(); i++)
			vkDestroyImageView(ctx->getLogicalDevice(), m_SwapchainImageViews[i], nullptr);
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	}

	void VulkanSwapchainManager::updateSwapchain() {
		auto& window = cbk::Application::get().getWindow();
		auto ctx = static_cast<VulkanDeviceContext*>(window.getContext());
		auto vkResult = vkDeviceWaitIdle(ctx->getLogicalDevice());
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error on ctx->getLogicalDevice() wait idle ({})", static_cast<int>(vkResult));
			return;
		}

		// Is this just to see if there is an error?
		vkResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->getPhysicalDevice(), m_Surface, &m_SurfaceCaps);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI::draw(): error getting surface capabilities ({})", static_cast<int>(vkResult));
			return;
		}

		m_SwapchainCI.oldSwapchain = m_Swapchain;
		m_SwapchainCI.imageExtent = { .width = static_cast<uint32_t>(window.getWidth()),
			                          .height = static_cast<uint32_t>(window.getHeight()) };
		vkResult = vkCreateSwapchainKHR(ctx->getLogicalDevice(), &m_SwapchainCI, nullptr, &m_Swapchain);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanRendererAPI: error creating swapchain ({})", static_cast<int>(vkResult));
			return;
		}

		for (auto i = 0; i < m_ImageCount; i++)
			vkDestroyImageView(ctx->getLogicalDevice(), m_SwapchainImageViews[i], nullptr);

		getSwapchainImages(ctx);
		vkDestroySwapchainKHR(ctx->getLogicalDevice(), m_SwapchainCI.oldSwapchain, nullptr);
		for (size_t i = 0; i < m_DepthImages.size(); i++) {
			vkDestroyImageView(ctx->getLogicalDevice(), m_DepthImageViews[i], nullptr);
			vmaDestroyImage(ctx->getAllocator(), m_DepthImages[i], m_DepthImageAllocations[i]);
		}
		m_DepthImages.clear();
		m_DepthImageViews.clear();
		m_DepthImageAllocations.clear();
		createDepthAttachment(ctx);
	}

	void VulkanSwapchainManager::createSwapchain(VulkanDeviceContext* ctx) {
		auto& window = cbk::Application::get().getWindow();
		auto glfwWindow = static_cast<GLFWwindow*>(window.getNativeWindow());
		VK_CHECK(glfwCreateWindowSurface(m_Instance, glfwWindow, nullptr, &m_Surface));
		CBK_CORE_ASSERT(m_Surface != VK_NULL_HANDLE, "VulkanSwapchainManager: surface is null after glfwCreateWindowSurface");
		ctx->selectSurfaceFormat(m_Surface);
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->getPhysicalDevice(), m_Surface, &m_SurfaceCaps));
		VkExtent2D swapchainExtent{ m_SurfaceCaps.currentExtent };
		if (m_SurfaceCaps.currentExtent.width == 0xFFFFFFFF) { // For wayland
			swapchainExtent = { .width = static_cast<uint32_t>(window.getWidth()), .height = static_cast<uint32_t>(window.getHeight()) };
		}

		VkSwapchainCreateInfoKHR swapchainCI{ .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			                                  .surface = m_Surface,
			                                  .minImageCount = m_SurfaceCaps.minImageCount,
			                                  .imageFormat = ctx->getImageFormat(),
			                                  .imageColorSpace = ctx->getImageColorSpace(),
			                                  .imageExtent{ .width = swapchainExtent.width, .height = swapchainExtent.height },
			                                  .imageArrayLayers = 1,
			                                  .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			                                  .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			                                  .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			                                  .presentMode = VK_PRESENT_MODE_FIFO_KHR };
		VK_CHECK(vkCreateSwapchainKHR(m_Device, &swapchainCI, nullptr, &m_Swapchain));

		// updateSwapchain() rebuilds from this, patching only oldSwapchain and the extent.
		// Without keeping it, recreation runs off a zeroed struct and fails on every resize.
		m_SwapchainCI = swapchainCI;
	}

	void VulkanSwapchainManager::getSwapchainImages(VulkanDeviceContext* ctx) {
		VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_ImageCount, nullptr));
		m_SwapchainImages.resize(m_ImageCount);
		VK_CHECK(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_ImageCount, m_SwapchainImages.data()));
		m_SwapchainImageViews.resize(m_ImageCount);
		for (auto i = 0; i < m_ImageCount; i++) {
			VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                          .image = m_SwapchainImages[i],
				                          .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                          .format = ctx->getImageFormat(),
				                          .subresourceRange{ .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
			VK_CHECK(vkCreateImageView(m_Device, &viewCI, nullptr, &m_SwapchainImageViews[i]));
		}
	}

	uint32_t VulkanSwapchainManager::getMinImageCount() const {
		return m_SurfaceCaps.minImageCount;
	}

	size_t VulkanSwapchainManager::getSwapchainImagesSize() const {
		return m_SwapchainImages.size();
	}

	VkSwapchainKHR* VulkanSwapchainManager::getSwapchain() {
		return &m_Swapchain;
	}

	VkImage VulkanSwapchainManager::getSwapchainImage(uint32_t index) const {
		return m_SwapchainImages.at(index);
	}

	VkImage VulkanSwapchainManager::getDepthImage(uint32_t index) const {
		return m_DepthImages.at(index);
	}

	VkImageView VulkanSwapchainManager::getSwapchainImageView(uint32_t index) const {
		return m_SwapchainImageViews.at(index);
	}

	VkImageView VulkanSwapchainManager::getDepthImageView(uint32_t index) const {
		return m_DepthImageViews.at(index);
	}

	void VulkanSwapchainManager::createDepthAttachment(VulkanDeviceContext* ctx) {
		auto& window = Application::get().getWindow();
		VkImageCreateInfo depthImageCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = ctx->getDepthFormat(),
			.extent{ .width = static_cast<uint32_t>(window.getWidth()), .height = static_cast<uint32_t>(window.getHeight()), .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VmaAllocationCreateInfo allocCI{ .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, .usage = VMA_MEMORY_USAGE_AUTO };

		// One depth image per swapchain image — pairs with the color attachment so
		// frames in flight can't race on a shared depth resource.
		m_DepthImages.resize(m_ImageCount);
		m_DepthImageViews.resize(m_ImageCount);
		m_DepthImageAllocations.resize(m_ImageCount);
		for (uint32_t i = 0; i < m_ImageCount; i++) {
			VK_CHECK(vmaCreateImage(ctx->getAllocator(), &depthImageCI, &allocCI, &m_DepthImages[i], &m_DepthImageAllocations[i], nullptr));
			VkImageViewCreateInfo depthViewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				                               .image = m_DepthImages[i],
				                               .viewType = VK_IMAGE_VIEW_TYPE_2D,
				                               .format = ctx->getDepthFormat(),
				                               .subresourceRange{
				                                   .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 } };
			VK_CHECK(vkCreateImageView(m_Device, &depthViewCI, nullptr, &m_DepthImageViews[i]));
		}
	}
} // namespace cbk::platform::vk
