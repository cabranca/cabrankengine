#include <pch.h>
#include "MetalStorageBuffer.h"

namespace cbk::platform::metal {

	// Allocate the full capacity up front (matching the size the Renderer reserves
	// for the light SSBO) so the backing store never reallocates on setData.
	MetalStorageBuffer::MetalStorageBuffer(uint32_t size) : m_Data(size, 0) {}

	MetalStorageBuffer::~MetalStorageBuffer() = default;

	void MetalStorageBuffer::setData(const void* data, uint32_t size) {
		if (size > m_Data.size())
			m_Data.resize(size);
		memcpy(m_Data.data(), data, size);
	}

	// The PBR material reads data()/size() and pushes the bytes via setFragmentBytes
	// at draw time, so there is nothing to bind here.
	void MetalStorageBuffer::bind(uint32_t /*bindingPoint*/) const {}
} // namespace cbk::platform::metal
