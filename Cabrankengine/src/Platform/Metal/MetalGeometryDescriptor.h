#pragma once

#include <Cabrankengine/Renderer/GeometryDescriptor.h>

namespace cbk::platform::metal {

    class MetalGeometryDescriptor : public rendering::GeometryDescriptor {
        public:
        MetalGeometryDescriptor(const void* vertexData, size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                                      const rendering::VertexLayout& layout);
        MetalGeometryDescriptor(size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                                      const rendering::VertexLayout& layout);

        void setData(const void* data, uint32_t size) override;
        uint32_t getIndexCount() const override;
    };
}