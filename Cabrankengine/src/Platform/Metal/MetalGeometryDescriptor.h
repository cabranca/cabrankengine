#pragma once

#include <Cabrankengine/Renderer/GeometryDescriptor.h>

namespace MTL {
	class Buffer;
    class RenderCommandEncoder;
}

namespace cbk::platform::metal {

	class MetalGeometryDescriptor : public rendering::GeometryDescriptor {
	  public:
		MetalGeometryDescriptor(const void* vertexData, size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                        const rendering::VertexLayout& layout);
		MetalGeometryDescriptor(size_t vertexDataSize, const void* indexData, size_t indexDataSize, const rendering::VertexLayout& layout);

		void setData(const void* data, uint32_t size) override;
		[[nodiscard]] uint32_t getIndexCount() const override;

		// Binds the combined buffer as the vertex buffer (offset 0). Indices live in
		// the same buffer at getIndexBufferOffset(); MetalRendererAPI passes that to
		// drawIndexedPrimitives.
		void bindBuffers(MTL::RenderCommandEncoder* encoder) const;
		[[nodiscard]] MTL::Buffer* getBuffer() const;
		[[nodiscard]] uint32_t getIndexBufferOffset() const;

	  private:
		MTL::Buffer* m_Buffer; // Metal buffer for the index buffer
		uint32_t m_VBufferSize;
		uint32_t m_IndexCount; // Number of indices in the index buffer
	};
} // namespace cbk::platform::metal