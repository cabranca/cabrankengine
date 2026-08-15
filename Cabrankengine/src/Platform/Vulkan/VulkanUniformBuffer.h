#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>


#include <Cabrankengine/Renderer/UniformBuffer.h>

#include "VulkanConstants.h"
#include "VulkanBuffer.h"
#include "VulkanDeviceContext.h"

namespace cbk::platform::vk {

	class VulkanUniformBuffer : public rendering::UniformBuffer {
	  public:
		void init(const VulkanDeviceContext& ctx, uint32_t size, uint32_t binding, VkStageFlags stageFlags);
		void shutdown();

		void setData(, uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset = 0) override;

		// Called by VulkanRendererAPI::beginFrame with the active frame index.
		// Determines which buffer setData writes to and which descriptor set
		// getDescriptorSet returns for the current frame.
		void setCurrentFrame(uint32_t frame) {
			m_CurrentFrame = frame % k_MaxFramesInFlight;
		}

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const {
			return m_DescriptorSetLayout;
		}
		[[nodiscard]] VkDescriptorSet getDescriptorSet(uint32_t index) const {
			return m_DescriptorSets[m_CurrentFrame];
		}

	  private:
		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		VmaAllocator m_Allocator = VK_NULL_HANDLE; // NON-OWNING
		std::array<VulkanBuffer, k_MaxFramesInFlight> m_Buffers{};
		VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorPool m_DescriptorPool{ VK_NULL_HANDLE };
		std::array<VkDescriptorSet, k_MaxFramesInFlight> m_DescriptorSets{};

		void createDescriptorSetLayout(VkStageFlags stageFlags);
		void createDescriptorPool();
		void allocateDescriptorSets();
		void updateDescriptorSets();
	};

} // namespace cbk::platform::vk
