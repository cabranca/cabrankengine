#include <pch.h>
#include "VulkanUniformBuffer.h"

#include <cstring>

#include "VkCheck.h"
#include "VulkanDeviceContext.h"
#include "VulkanRendererAPI.h"

namespace cbk::platform::vk {

	VulkanUniformBuffer::VulkanUniformBuffer(uint32_t size, uint32_t binding) : m_Size(size) {
		auto ctx = &VulkanRendererAPI::getContext();
		auto device = ctx->getDevice();

		// 1) Per-slot persistently mapped host-visible buffer. Independent allocations
		// so the GPU can read slot N-1 while the CPU writes slot N.
		VkBufferCreateInfo bufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		};
		VmaAllocationCreateInfo allocCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			         VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};
		for (uint32_t i = 0; i < k_MaxFramesInFlight; ++i)
			VK_CHECK(vmaCreateBuffer(ctx->getAllocator(), &bufferCI, &allocCI, &m_Buffers[i], &m_Allocations[i], &m_AllocationInfos[i]));

		// 2) One-binding descriptor set layout, accessible to both stages.
		VkDescriptorSetLayoutBinding layoutBinding{
			.binding = binding,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		};
		VkDescriptorSetLayoutCreateInfo layoutCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &layoutBinding,
		};
		VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescriptorSetLayout));

		// 3) Pool with k_MaxFramesInFlight descriptor sets, one per buffer.
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = k_MaxFramesInFlight,
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = k_MaxFramesInFlight,
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize,
		};
		VK_CHECK(vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescriptorPool));

		std::array<VkDescriptorSetLayout, k_MaxFramesInFlight> setLayouts;
		setLayouts.fill(m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo dsAllocCI{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = m_DescriptorPool,
			.descriptorSetCount = k_MaxFramesInFlight,
			.pSetLayouts = setLayouts.data(),
		};
		VK_CHECK(vkAllocateDescriptorSets(device, &dsAllocCI, m_DescriptorSets.data()));

		// Point each descriptor set at its own buffer.
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

	VulkanUniformBuffer::~VulkanUniformBuffer() {
		auto ctx = &VulkanRendererAPI::getContext();
		auto device = ctx->getDevice();

		// The descriptor pool and buffers may still be referenced by in-flight
		// command buffers from the final frame(s). Wait for the GPU to drain
		// before destroying them.
		vkDeviceWaitIdle(device);

		if (m_DescriptorPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
		if (m_DescriptorSetLayout != VK_NULL_HANDLE)
			vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
		for (uint32_t i = 0; i < k_MaxFramesInFlight; ++i) {
			if (m_Buffers[i] != VK_NULL_HANDLE)
				vmaDestroyBuffer(ctx->getAllocator(), m_Buffers[i], m_Allocations[i]);
		}
	}

	void VulkanUniformBuffer::setData(const void* data, uint32_t size, uint32_t offset) {
		auto mapped = m_AllocationInfos[m_CurrentFrame].pMappedData;
		if (!mapped) {
			CBK_CORE_ERROR("VulkanUniformBuffer::setData: buffer slot {} is not mapped", m_CurrentFrame);
			return;
		}
		std::memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);
	}

} // namespace cbk::platform::vk
