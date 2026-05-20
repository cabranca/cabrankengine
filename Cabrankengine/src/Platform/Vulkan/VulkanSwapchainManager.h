#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace cbk::platform::vk {

	class VulkanDeviceContext;

	class VulkanSwapchainManager {
	  public:
		void init(VulkanDeviceContext* vkDeviceContext);
		void shutdown();

		void updateSwapchain();
		
		[[nodiscard]] uint32_t getMinImageCount() const;
		[[nodiscard]] size_t getSwapchainImagesSize() const;
		[[nodiscard]] VkSwapchainKHR* getSwapchain();
		[[nodiscard]] VkImage getSwapchainImage(uint32_t index) const;
		[[nodiscard]] VkImage getDepthImage(uint32_t index) const;
		[[nodiscard]] VkImageView getSwapchainImageView(uint32_t index) const;
		[[nodiscard]] VkImageView getDepthImageView(uint32_t index) const;

	  private:
        VkInstance m_Instance; // OWNED BY VULKANDEVICECONTEXT
		VkDevice m_Device; // OWNED BY VULKANDEVICECONTEXT

        // --- Swapchain ---
		VkSurfaceKHR m_Surface{ VK_NULL_HANDLE };
		VkSurfaceCapabilitiesKHR m_SurfaceCaps{};
		VkSwapchainCreateInfoKHR m_SwapchainCI{};
		VkSwapchainKHR m_Swapchain{ VK_NULL_HANDLE };
		uint32_t m_ImageCount{ 0 };
		
        // Color Buffer
        std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;

		// --- Depth Buffer ---
		// One per swapchain image so depth doesn't race across frames in flight.
		std::vector<VmaAllocation> m_DepthImageAllocations;
		std::vector<VkImage>       m_DepthImages;
		std::vector<VkImageView>   m_DepthImageViews;

		bool createSwapchain(VulkanDeviceContext* ctx);
		bool getSwapchainImages(VulkanDeviceContext* ctx);
		bool createDepthAttachment(VulkanDeviceContext* ctx);
	};
} // namespace cbk::platform::vk