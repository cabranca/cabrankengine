#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Core/Core.h>
#include <Cabrankengine/Renderer/UniformBuffer.h>

namespace cbk::platform::vk {

	class VulkanUniformBuffer : public rendering::UniformBuffer {
	  public:
		// Must match VulkanRendererAPI::k_MaxFramesInFlight. N-buffered so that the
		// CPU never overwrites a UBO that the GPU is still reading from for the prior frame.
		static constexpr uint32_t k_Slots = 2;

		VulkanUniformBuffer(uint32_t size, uint32_t binding);
		~VulkanUniformBuffer() override;

		void setData(const void* data, uint32_t size, uint32_t offset = 0) override;

		// Called by VulkanRendererAPI::beginFrame with the active frame-in-flight
		// index. Determines which slot setData writes to and which descriptor set
		// getDescriptorSet returns.
		void setCurrentSlot(uint32_t slot) {
			m_CurrentSlot = slot % k_Slots;
		}

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const {
			return m_DescriptorSetLayout;
		}
		[[nodiscard]] const VkDescriptorSet* getDescriptorSet() const {
			return &m_DescriptorSets[m_CurrentSlot];
		}

	  private:
		std::array<VkBuffer, k_Slots>          m_Buffers{};
		std::array<VmaAllocation, k_Slots>     m_Allocations{};
		std::array<VmaAllocationInfo, k_Slots> m_AllocationInfos{};
		VkDescriptorSetLayout                  m_DescriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorPool                       m_DescriptorPool{ VK_NULL_HANDLE };
		std::array<VkDescriptorSet, k_Slots>   m_DescriptorSets{};
		uint32_t                               m_Size{ 0 };
		uint32_t                               m_CurrentSlot{ 0 };
	};

} // namespace cbk::platform::vk
