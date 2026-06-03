// #include <pch.h>

// #include <Metal/Metal.hpp>

// #include <Cabrankengine/Core/Application.h>
// #include <Cabrankengine/Core/Window.h>

// #include "MetalBuffer.h"
// #include "Metal/MTLResource.hpp"
// #include "MetalContext.h"

// namespace cbk::platform::metal {

// 	// --------------------- VERTEX BUFFER --------------------- //

// 	MetalVertexBuffer::MetalVertexBuffer(uint32_t size) : m_Size(size) {
// 		CBK_PROFILE_FUNCTION();

// 		MetalContext* context = static_cast<MetalContext*>(Application::get().getWindow().getContext());
// 		m_Buffer = context->getDevice()->newBuffer(size, MTL::ResourceStorageModeShared);
// 	}

// 	MetalVertexBuffer::MetalVertexBuffer(void* vertices, uint32_t size) : m_Size(size) {
// 		CBK_PROFILE_FUNCTION();

// 		auto* context = static_cast<MetalContext*>(Application::get().getWindow().getContext());
// 		m_Buffer = context->getDevice()->newBuffer(vertices, size, MTL::ResourceStorageModeShared);
// 	}

// 	MetalVertexBuffer::~MetalVertexBuffer() {
// 		CBK_PROFILE_FUNCTION();

// 		if (m_Buffer)
// 			m_Buffer->release();
// 	}

// 	void MetalVertexBuffer::bind() const {}

// 	void MetalVertexBuffer::unbind() const {}

// 	void MetalVertexBuffer::setData(const void* data, uint32_t size) {
// 		CBK_PROFILE_FUNCTION();

// 		memcpy(m_Buffer->contents(), data, size);
// 	}

// 	// --------------------- INDEX BUFFER --------------------- //

// 	MetalIndexBuffer::MetalIndexBuffer(uint32_t* indices, uint32_t count) : m_Count(count) {
// 		CBK_PROFILE_FUNCTION();

// 		MetalContext* context = static_cast<MetalContext*>(Application::get().getWindow().getContext());
// 		m_Buffer = context->getDevice()->newBuffer(indices, count * sizeof(uint32_t), MTL::ResourceStorageModeShared);
// 	}

// 	MetalIndexBuffer::~MetalIndexBuffer() {
// 		CBK_PROFILE_FUNCTION();

// 		if (m_Buffer)
// 			m_Buffer->release();
// 	}

// 	void MetalIndexBuffer::bind() const {}

// 	void MetalIndexBuffer::unbind() const {}
// } // namespace cbk::platform::metal
