#include <pch.h>
#include "VulkanDescriptorBuffer.h"

#include "VkCheck.h"

namespace cbk::platform::vk {

	// ----------STORAGE BUFFER----------

	void VulkanStorageBuffer::init(VkDevice device, VmaAllocator allocator, uint32_t size, uint32_t binding,
	                               VkShaderStageFlags stageFlags) {
		m_DescriptorBuffer.init(device, allocator, size, binding, stageFlags, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		                        VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT);
	}

	void VulkanStorageBuffer::shutdown() {
		m_DescriptorBuffer.shutdown();
	}

	void VulkanStorageBuffer::setData(uint32_t frameIndex, const void* data, uint32_t size) {
		m_DescriptorBuffer.setData(frameIndex, data, size, 0);
	}

	// ----------UNIFORM BUFFER----------

	void VulkanUniformBuffer::init(VkDevice device, VmaAllocator allocator, uint32_t size, uint32_t binding,
	                               VkShaderStageFlags stageFlags) {
		m_DescriptorBuffer.init(device, allocator, size, binding, stageFlags, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                        VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT);
	}

	void VulkanUniformBuffer::shutdown() {
		m_DescriptorBuffer.shutdown();
	}

	void VulkanUniformBuffer::setData(uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset) {
		m_DescriptorBuffer.setData(frameIndex, data, size, offset);
	}

	// ----------DESCRIPTOR BUFFER----------

	void VulkanDescriptorBuffer::init(VkDevice device, VmaAllocator allocator, uint32_t size, uint32_t binding, VkShaderStageFlags stageFlags,
									  VkDescriptorType descriptorType, VkBufferUsageFlags usage) {
		m_Device = device;
		m_Allocator = allocator;
		m_DescriptorType = descriptorType;

		for (auto& buffer : m_Buffers) {
			buffer.init(m_Device, m_Allocator, size, usage);
		}

		createDescriptorSetLayout(binding, stageFlags);
		createDescriptorPool();
		allocateDescriptorSets();
		updateDescriptorSets(size, binding);
	}

	void VulkanDescriptorBuffer::shutdown() {
		if (m_DescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		if (m_DescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
		for (auto& buffer : m_Buffers) {
			buffer.shutdown();
		}
	}

	void VulkanDescriptorBuffer::setData(uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset) {
		m_Buffers[frameIndex].setData(data, size, offset);
	}

	void VulkanDescriptorBuffer::createDescriptorSetLayout(uint32_t binding, VkShaderStageFlags stageFlags) {
		VkDescriptorSetLayoutBinding layoutBinding{
			.binding = binding,
			.descriptorType = m_DescriptorType,
			.descriptorCount = 1,
			.stageFlags = stageFlags,
		};
		VkDescriptorSetLayoutCreateInfo layoutCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &layoutBinding,
		};
		VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutCI, nullptr, &m_DescriptorSetLayout));
	}

	void VulkanDescriptorBuffer::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{
			.type = m_DescriptorType,
			.descriptorCount = k_MaxFramesInFlight,
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = k_MaxFramesInFlight, // TODO: check that this amount is correct
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		VK_CHECK(vkCreateDescriptorPool(m_Device, &poolCI, nullptr, &m_DescriptorPool));
	}

	void VulkanDescriptorBuffer::allocateDescriptorSets() {
		std::array<VkDescriptorSetLayout, k_MaxFramesInFlight> setLayouts;
		setLayouts.fill(m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo dsAllocCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_DescriptorPool,
			.descriptorSetCount = k_MaxFramesInFlight,
			.pSetLayouts = setLayouts.data(),
		};
		VK_CHECK(vkAllocateDescriptorSets(m_Device, &dsAllocCI, m_DescriptorSets.data()));
	}

	void VulkanDescriptorBuffer::updateDescriptorSets(uint32_t size, uint32_t binding) {
		std::array<VkDescriptorBufferInfo, k_MaxFramesInFlight> bufferInfos{};
		std::array<VkWriteDescriptorSet, k_MaxFramesInFlight> writes{};
		for (uint32_t i = 0; i < k_MaxFramesInFlight; ++i) {
			bufferInfos[i] = VkDescriptorBufferInfo{
				.buffer = m_Buffers[i].getBuffer(),
				.offset = 0,
				.range = size,
			};
			writes[i] = VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSets[i],
				.dstBinding = binding,
				.descriptorCount = 1,
				.descriptorType = m_DescriptorType,
				.pBufferInfo = &bufferInfos[i],
			};
		}
		vkUpdateDescriptorSets(m_Device, k_MaxFramesInFlight, writes.data(), 0, nullptr);
	}
} // namespace cbk::platform::vk
