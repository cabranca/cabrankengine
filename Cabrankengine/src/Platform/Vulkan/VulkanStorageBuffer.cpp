#include <pch.h>
#include "VulkanStorageBuffer.h"

#include <cstring>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "VulkanDeviceContext.h"

namespace cbk::platform::vk {

	VulkanStorageBuffer::VulkanStorageBuffer(uint32_t size, uint32_t binding) : m_Size(size) {
		auto ctx    = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		auto device = ctx->getLogicalDevice();

		// 1) Per-slot persistently mapped host-visible buffer. Independent allocations
		// so the GPU can read slot N-1 while the CPU writes slot N.
		VkBufferCreateInfo bufferCI{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size  = size,
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		};
		VmaAllocationCreateInfo allocCI{
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			         VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			         VMA_ALLOCATION_CREATE_MAPPED_BIT,
			.usage = VMA_MEMORY_USAGE_AUTO,
		};
		for (uint32_t i = 0; i < k_MaxFramesInFlight; ++i) {
			auto vkResult = vmaCreateBuffer(ctx->getAllocator(), &bufferCI, &allocCI, &m_Buffers[i], &m_Allocations[i], &m_AllocationInfos[i]);
			if (vkResult != VK_SUCCESS) {
				CBK_CORE_ERROR("VulkanStorageBuffer: vmaCreateBuffer slot {} failed ({})", i, static_cast<int>(vkResult));
				return;
			}
		}

		// 2) One-binding descriptor set layout. Only the fragment stage reads point
		// lights today (PBR shading happens there); widen the stage mask if a future
		// pass needs the SSBO from another stage.
		VkDescriptorSetLayoutBinding layoutBinding{
			.binding         = binding,
			.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
		};
		VkDescriptorSetLayoutCreateInfo layoutCI{
			.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings    = &layoutBinding,
		};
		auto vkResult = vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &m_DescriptorSetLayout);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanStorageBuffer: vkCreateDescriptorSetLayout failed ({})", static_cast<int>(vkResult));
			return;
		}

		// 3) Pool with k_MaxFramesInFlight descriptor sets, one per buffer.
		VkDescriptorPoolSize poolSize{
			.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = k_MaxFramesInFlight,
		};
		VkDescriptorPoolCreateInfo poolCI{
			.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets       = k_MaxFramesInFlight,
			.poolSizeCount = 1,
			.pPoolSizes    = &poolSize,
		};
		vkResult = vkCreateDescriptorPool(device, &poolCI, nullptr, &m_DescriptorPool);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanStorageBuffer: vkCreateDescriptorPool failed ({})", static_cast<int>(vkResult));
			return;
		}

		std::array<VkDescriptorSetLayout, k_MaxFramesInFlight> setLayouts;
		setLayouts.fill(m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo dsAllocCI{
			.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool     = m_DescriptorPool,
			.descriptorSetCount = k_MaxFramesInFlight,
			.pSetLayouts        = setLayouts.data(),
		};
		vkResult = vkAllocateDescriptorSets(device, &dsAllocCI, m_DescriptorSets.data());
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ERROR("VulkanStorageBuffer: vkAllocateDescriptorSets failed ({})", static_cast<int>(vkResult));
			return;
		}

		// Point each descriptor set at its own buffer.
		std::array<VkDescriptorBufferInfo, k_MaxFramesInFlight> bufferInfos{};
		std::array<VkWriteDescriptorSet, k_MaxFramesInFlight>   writes{};
		for (uint32_t i = 0; i < k_MaxFramesInFlight; ++i) {
			bufferInfos[i] = VkDescriptorBufferInfo{
				.buffer = m_Buffers[i],
				.offset = 0,
				.range  = size,
			};
			writes[i] = VkWriteDescriptorSet{
				.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet          = m_DescriptorSets[i],
				.dstBinding      = binding,
				.descriptorCount = 1,
				.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo     = &bufferInfos[i],
			};
		}
		vkUpdateDescriptorSets(device, k_MaxFramesInFlight, writes.data(), 0, nullptr);
	}

	VulkanStorageBuffer::~VulkanStorageBuffer() {
		auto ctx    = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
		auto device = ctx->getLogicalDevice();

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

	void VulkanStorageBuffer::setData(const void* data, uint32_t size) {
		auto mapped = m_AllocationInfos[m_CurrentFrame].pMappedData;
		if (!mapped) {
			CBK_CORE_ERROR("VulkanStorageBuffer::setData: buffer slot {} is not mapped", m_CurrentFrame);
			return;
		}
		std::memcpy(mapped, data, size);
	}

} // namespace cbk::platform::vk
