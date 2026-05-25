#include <pch.h>
#include "VulkanGeometryDescriptor.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "VulkanDeviceContext.h"

namespace cbk::platform::vk {

	VulkanGeometryDescriptor::VulkanGeometryDescriptor(const void* vertexData, size_t vertexDataSize, const void* indexData,
	                                                   size_t indexDataSize, const rendering::VertexLayout& layout)
	    : m_VBSize(vertexDataSize), m_IndexCount(indexDataSize / sizeof(uint32_t)) {

		VmaAllocator allocator = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext())->getAllocator();
		VkBufferCreateInfo bufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			                         .size = vertexDataSize + indexDataSize,
			                         .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT };
		VmaAllocationCreateInfo vBufferAllocCI{ .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			                                             VMA_ALLOCATION_CREATE_MAPPED_BIT,
			                                    .usage = VMA_MEMORY_USAGE_AUTO };
		auto vkResult = vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &m_VBuffer, &m_VBufferAllocation, &m_VBufferAllocInfo);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ASSERT("VulkanGeometryDescriptor(): error creating Vertex Buffer", static_cast<int>(vkResult));
			return;
		}
		memcpy(m_VBufferAllocInfo.pMappedData, vertexData, vertexDataSize);
		memcpy(((char*)m_VBufferAllocInfo.pMappedData) + vertexDataSize, indexData, indexDataSize);
	}

    VulkanGeometryDescriptor::VulkanGeometryDescriptor(size_t vertexDataSize, const void* indexData,
	                                                   size_t indexDataSize, const rendering::VertexLayout& layout) : m_VBSize(vertexDataSize), m_IndexCount(indexDataSize / sizeof(uint32_t)) {

		VmaAllocator allocator = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext())->getAllocator();
		VkBufferCreateInfo bufferCI{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			                         .size = vertexDataSize + indexDataSize,
			                         .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT };
		VmaAllocationCreateInfo vBufferAllocCI{ .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			                                             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			                                             VMA_ALLOCATION_CREATE_MAPPED_BIT,
			                                    .usage = VMA_MEMORY_USAGE_AUTO };
		auto vkResult = vmaCreateBuffer(allocator, &bufferCI, &vBufferAllocCI, &m_VBuffer, &m_VBufferAllocation, &m_VBufferAllocInfo);
		if (vkResult != VK_SUCCESS) {
			CBK_CORE_ASSERT("VulkanGeometryDescriptor(): error creating Vertex Buffer", static_cast<int>(vkResult));
			return;
		}
		memcpy(((char*)m_VBufferAllocInfo.pMappedData) + vertexDataSize, indexData, indexDataSize);
	}

	VulkanGeometryDescriptor::~VulkanGeometryDescriptor() {
		shutdown();
	}

	void VulkanGeometryDescriptor::shutdown() {
		if (m_VBuffer == VK_NULL_HANDLE)
			return;
		VmaAllocator allocator = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext())->getAllocator();
		vmaDestroyBuffer(allocator, m_VBuffer, m_VBufferAllocation);
	}

	void VulkanGeometryDescriptor::setData(const void* data, uint32_t size) {
		memcpy(m_VBufferAllocInfo.pMappedData, data, size);
	}

	uint32_t VulkanGeometryDescriptor::getIndexCount() const {
		return m_IndexCount;
	}

	const VkBuffer* VulkanGeometryDescriptor::getBuffer() const {
        return &m_VBuffer;
    }

	void VulkanGeometryDescriptor::bindBuffers(VkCommandBuffer cb) {
		VkDeviceSize vOffset{ 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, &m_VBuffer, &vOffset);
		vkCmdBindIndexBuffer(cb, m_VBuffer, m_VBSize, VK_INDEX_TYPE_UINT32);
	}
} // namespace cbk::platform::vk