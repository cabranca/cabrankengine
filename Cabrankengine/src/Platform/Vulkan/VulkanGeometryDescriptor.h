#pragma once

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

#include <Cabrankengine/Renderer/GeometryDescriptor.h>

namespace cbk::platform::vk {

	class VulkanGeometryDescriptor : public rendering::GeometryDescriptor {
	  public:
		VulkanGeometryDescriptor(const void* vertexData, size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                         const rendering::VertexLayout& layout);
		VulkanGeometryDescriptor(size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                         const rendering::VertexLayout& layout);

		~VulkanGeometryDescriptor();
		void shutdown();
		
		void setData(const void* data, uint32_t size) override;

		uint32_t getIndexCount() const override;

		[[nodiscard]] const VkBuffer* getBuffer() const;
		void bindBuffers(VkCommandBuffer cb);

	  private:
		VkDeviceSize m_VBSize;
		VkBuffer m_VBuffer{ VK_NULL_HANDLE };
		uint32_t m_IndexCount;
		VmaAllocationInfo m_VBufferAllocInfo{};
		VmaAllocation m_VBufferAllocation;
	};
} // namespace cbk::platform::vk