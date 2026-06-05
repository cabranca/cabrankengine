#include <pch.h>
#include "MetalGeometryDescriptor.h"

#include <Metal/Metal.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "MetalBinding.h"
#include "MetalDeviceContext.h"

namespace cbk::platform::metal {

	using namespace rendering;

	MetalGeometryDescriptor::MetalGeometryDescriptor(const void* vertexData, size_t vertexDataSize, const void* indexData,
	                                                 size_t indexDataSize, const VertexLayout& layout)
	    : m_VBufferSize(static_cast<uint32_t>(vertexDataSize)), m_IndexCount(static_cast<uint32_t>(indexDataSize / sizeof(uint32_t))) {
		auto context = static_cast<MetalDeviceContext*>(Application::get().getWindow().getContext());
		// One shared buffer holds [vertices | indices]; vertices at offset 0, indices
		// at offset m_VBufferSize — mirroring VulkanGeometryDescriptor's single buffer.
		m_Buffer = context->getDevice()->newBuffer(vertexDataSize + indexDataSize, MTL::ResourceStorageModeShared);
		char* dst = static_cast<char*>(m_Buffer->contents());
		memcpy(dst, vertexData, vertexDataSize);
		memcpy(dst + vertexDataSize, indexData, indexDataSize);
	}

	MetalGeometryDescriptor::MetalGeometryDescriptor(size_t vertexDataSize, const void* indexData, size_t indexDataSize,
	                                                 const VertexLayout& layout)
	    : m_VBufferSize(static_cast<uint32_t>(vertexDataSize)), m_IndexCount(static_cast<uint32_t>(indexDataSize / sizeof(uint32_t))) {
		auto context = static_cast<MetalDeviceContext*>(Application::get().getWindow().getContext());
		// Vertex region is left uninitialised here (filled later via setData by the
		// batchers); only the index region is uploaded up front.
		m_Buffer = context->getDevice()->newBuffer(vertexDataSize + indexDataSize, MTL::ResourceStorageModeShared);
		char* dst = static_cast<char*>(m_Buffer->contents());
		memcpy(dst + vertexDataSize, indexData, indexDataSize);
	}

	void MetalGeometryDescriptor::setData(const void* data, uint32_t size) {
		// Writes into the vertex region (offset 0). Shared storage means the GPU sees
		// the update without an explicit flush on Apple Silicon.
		memcpy(m_Buffer->contents(), data, size);
	}

	uint32_t MetalGeometryDescriptor::getIndexCount() const {
		return m_IndexCount;
	}

	void MetalGeometryDescriptor::bindBuffers(MTL::RenderCommandEncoder* encoder) const {
		encoder->setVertexBuffer(m_Buffer, 0, buffer_index::k_VertexData);
	}

	MTL::Buffer* MetalGeometryDescriptor::getBuffer() const {
		return m_Buffer;
	}

	uint32_t MetalGeometryDescriptor::getIndexBufferOffset() const {
		return m_VBufferSize;
	}
} // namespace cbk::platform::metal
