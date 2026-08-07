#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Core/Core.h>
#include <Cabrankengine/Renderer/UniformBuffer.h>

#include "VulkanConstants.h"

namespace cbk::platform::vk {

	class VulkanUniformBuffer : public rendering::UniformBuffer {
	  public:
		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		~VulkanUniformBuffer();

		void setData(const void* data, uint32_t size, uint32_t offset = 0) override;

		// Called by VulkanRendererAPI::beginFrame with the active frame index.
		// Determines which buffer setData writes to and which descriptor set
		// getDescriptorSet returns for the current frame.
		void setCurrentFrame(uint32_t frame) {
			m_CurrentFrame = frame % k_MaxFramesInFlight;
		}

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const {
			return m_DescriptorSetLayout;
		}
		[[nodiscard]] const VkDescriptorSet* getDescriptorSet() const {
			return &m_DescriptorSets[m_CurrentFrame];
		}

	  private:
		std::array<VkBuffer, k_MaxFramesInFlight> m_Buffers{};
		std::array<VmaAllocation, k_MaxFramesInFlight> m_Allocations{};
		std::array<VmaAllocationInfo, k_MaxFramesInFlight> m_AllocationInfos{};
		VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorPool m_DescriptorPool{ VK_NULL_HANDLE };
		std::array<VkDescriptorSet, k_MaxFramesInFlight> m_DescriptorSets{};
		uint32_t m_Size{ 0 };
		uint32_t m_CurrentFrame{ 0 };
	};

} // namespace cbk::platform::vk
