#include <pch.h>
#include "VulkanUniformBuffer.h"

#include <cstring>

#include "VkCheck.h"
#include "VulkanDeviceContext.h"
#include "VulkanRendererAPI.h"

namespace cbk::platform::vk {

	void VulkanUniformBuffer::init(const VulkanDeviceContext& ctx, uint32_t size, uint32_t binding, VkStageFlags stageFlags) {
		m_Device = ctx.getDevice();
		m_Allocator = ctx.getAllocator();

		for (const auto& buffer : m_Buffers) {
			buffer.init(m_Device, m_Allocator, VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT);
		}
		createDescriptorSetLayout(stageFlags);
		createDescriptorPool();
		allocateDescriptorSets();
		updateDescriptorSets();
	}

	void VulkanUniformBuffer::shutdown() {
		if (m_DescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
		if (m_DescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
		for (const auto& buffer : m_Buffers) {
			buffer.shutdown();
		}
	}

	void VulkanUniformBuffer::setData(uint32_t frameIndex, const void* data, uint32_t size, uint32_t offset) {
		m_Buffers[frameIndex].setData(data, size, offset);
	}

	void VulkanUniformBuffer::createDescriptorSetLayout(VkStageFlags stageFlags) {
		VkDescriptorSetLayoutBinding layoutBinding{
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
	
	void VulkanUniformBuffer::createDescriptorPool() {
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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

	void VulkanUniformBuffer::allocateDescriptorSets() {
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

	void VulkanUniformBuffer::updateDescriptorSets() {
		std::array<VkDescriptorBufferInfo, k_MaxFramesInFlight> bufferInfos{};
		std::array<VkWriteDescriptorSet, k_MaxFramesInFlight> writes{};
		for (uint32_t i = 0; i < k_MaxFramesInFlight; ++i) {
			bufferInfos[i] = VkDescriptorBufferInfo{
				.buffer = m_Buffers[i],
				.offset = 0,
				.range = size,
			};
			writes[i] = VkWriteDescriptorSet{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSets[i],
				.dstBinding = binding,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfos[i],
			};
		}
		vkUpdateDescriptorSets(device, k_MaxFramesInFlight, writes.data(), 0, nullptr);
	}
} // namespace cbk::platform::vk
