#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Core/Core.h>
#include <Cabrankengine/Renderer/StorageBuffer.h>

#include "VulkanConstants.h"

namespace cbk::platform::vk {

	class VulkanStorageBuffer : public rendering::StorageBuffer {
	  public:
		VulkanStorageBuffer(uint32_t size, uint32_t binding);
		~VulkanStorageBuffer() override;

		void setData(const void* data, uint32_t size) override;

		// Vulkan binds the SSBO at draw time through the per-frame descriptor set
		// (see VulkanRendererAPI::commitRenderCommands), so the cross-API bind()
		// hook has nothing to do here. Kept as an override so the OpenGL call site
		// stays unchanged.
		void bind(uint32_t /*bindingPoint*/) const override {}

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
