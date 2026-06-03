#include <pch.h>
#include "MetalGeometryDescriptor.h"

namespace cbk::platform::metal {

    using namespace rendering;

    MetalGeometryDescriptor::MetalGeometryDescriptor(const void* vertexData, size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                                      const VertexLayout& layout) {

                                              }
        MetalGeometryDescriptor::MetalGeometryDescriptor(size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                                      const VertexLayout& layout) {

                                              }

        void MetalGeometryDescriptor::setData(const void* data, uint32_t size) {

        }
        uint32_t MetalGeometryDescriptor::getIndexCount() const {
            return 0;
        }
}