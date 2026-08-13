#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <array>
#include <vector>

#include "VulkanConstants.h"

namespace cbk::platform::vk {

	class VulkanDeviceContext;

	class VulkanSwapchainManager {
	  public:
		void init(const VulkanDeviceContext& vkDeviceContext);
		void shutdown();

		uint32_t acquireImage(VkSemaphore presentCompleteSempahore);
		void recreateSwapchain();

		[[nodiscard]] VkSwapchainKHR getSwapchain();
		[[nodiscard]] uint32_t getMinImageCount() const;
		[[nodiscard]] size_t getSwapchainImagesSize() const;
		[[nodiscard]] VkImage getSwapchainImage(uint32_t index) const;
		[[nodiscard]] VkImageView getSwapchainImageView(uint32_t index) const;
		[[nodiscard]] VkSemaphore getSemaphore(uint32_t index) const;
		[[nodiscard]] VkExtent2D getExtent() const;

		// Per-frame-in-flight scene render targets, indexed by frame-in-flight slot rather
		// than swapchain image count. Up to k_MaxFramesInFlight frames can have GPU work in
		// flight simultaneously, so sharing one color/depth image across all of them would
		// let one frame's attachment writes race the previous frame's still-executing ones.
		[[nodiscard]] VkImage getColorImage(uint32_t frameIndex) const;
		[[nodiscard]] VkImage getDepthImage(uint32_t frameIndex) const;
		[[nodiscard]] VkImageView getColorImageView(uint32_t frameIndex) const;
		[[nodiscard]] VkImageView getDepthImageView(uint32_t frameIndex) const;

	  private:
		// TODO: consider owning a function pointer to a capabilities getter because that's all I need to recreate thw swapchain
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE; // NON-OWNING
		VkDevice m_Device = VK_NULL_HANDLE;                 // NON-OWNING
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;            // NON-OWNING
		uint32_t m_QueueFamilyIndex = 0;
		VmaAllocator m_Allocator = VK_NULL_HANDLE; // NON-OWNING

		// --- Swapchain ---
		VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };
		VkSurfaceFormatKHR m_SelectedFormat;
		VkExtent2D m_Extent;
		VkFormat m_DepthFormat;
		uint32_t m_MinImageCount = 0;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;

		// Swapchain Images
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		// Color Attachment (offscreen; sampled by ImGui), one per frame in flight.
		std::array<VkImage, k_MaxFramesInFlight> m_ColorImages{};
		std::array<VkImageView, k_MaxFramesInFlight> m_ColorImageViews{};
		std::array<VmaAllocation, k_MaxFramesInFlight> m_ColorImageAllocations{};

		// Depth Attachment, one per frame in flight.
		std::array<VkImage, k_MaxFramesInFlight> m_DepthImages{};
		std::array<VkImageView, k_MaxFramesInFlight> m_DepthImageViews{};
		std::array<VmaAllocation, k_MaxFramesInFlight> m_DepthImageAllocations{};

		void createSwapchain(VkSwapchainKHR oldSwapchain);
		[[nodiscard]] static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		[[nodiscard]] static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		[[nodiscard]] VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		[[nodiscard]] static uint32_t chooseSwapMinImageCount(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
		void createImageViews();
		void createRenderFinishedSemaphores();
		void createRenderTargets();
		void destroyRenderTargets();
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		void cleanupSwapchain();
	};
} // namespace cbk::platform::vk