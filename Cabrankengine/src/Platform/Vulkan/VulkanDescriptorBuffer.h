#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Renderer/StorageBuffer.h>
#include <Cabrankengine/Renderer/UniformBuffer.h>

#include "VulkanConstants.h"
#include "VulkanBuffer.h"

namespace cbk::platform::vk {

	class VulkanDescriptorBuffer {
	  public:
		void init(VkDevice device, VmaAllocator allocator, uint32_t size, uint32_t binding, VkShaderStageFlags stageFlags,
		          VkDescriptorType descriptorType, VkBufferUsageFlags usage);
		void shutdown();
		void setData(uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset);

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_DescriptorSetLayout; }
		[[nodiscard]] VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {	return m_DescriptorSets[frameIndex]; }

	  private:
		VkDevice m_Device = VK_NULL_HANDLE;        // NON-OWNING
		VmaAllocator m_Allocator = VK_NULL_HANDLE; // NON-OWNING
		std::array<VulkanBuffer, k_MaxFramesInFlight> m_Buffers{};
		VkDescriptorSetLayout m_DescriptorSetLayout{ VK_NULL_HANDLE };
		VkDescriptorPool m_DescriptorPool{ VK_NULL_HANDLE };
		std::array<VkDescriptorSet, k_MaxFramesInFlight> m_DescriptorSets{};
		VkDescriptorType m_DescriptorType;

		void createDescriptorSetLayout(uint32_t binding, VkShaderStageFlags stageFlags);
		void createDescriptorPool();
		void allocateDescriptorSets();
		void updateDescriptorSets(uint32_t size, uint32_t binding);
	};

	class VulkanStorageBuffer : public rendering::StorageBuffer {
	  public:
	  	void init(VkDevice device, VmaAllocator allocator, uint32_t size, uint32_t binding, VkShaderStageFlags stageFlags);
		void shutdown();
		void setData(uint32_t frameIndex, const void* data, uint32_t size) override;

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_DescriptorBuffer.getDescriptorSetLayout(); }
		[[nodiscard]] VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {	return m_DescriptorBuffer.getDescriptorSet(frameIndex); }

	  private:
		VulkanDescriptorBuffer m_DescriptorBuffer;
	};

	class VulkanUniformBuffer : public rendering::UniformBuffer {
	  public:
	  	void init(VkDevice device, VmaAllocator allocator, uint32_t size, uint32_t binding, VkShaderStageFlags stageFlags);
		void shutdown();
		void setData(uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset) override;

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return m_DescriptorBuffer.getDescriptorSetLayout(); }
		[[nodiscard]] VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const {	return m_DescriptorBuffer.getDescriptorSet(frameIndex); }

	  private:
		VulkanDescriptorBuffer m_DescriptorBuffer;
	};

} // namespace cbk::platform::vk
