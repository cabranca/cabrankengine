// #include <pch.h>

// #include <Metal/Metal.hpp>

// #include <Cabrankengine/Renderer/Buffer.h>

// #include "MetalVertexArray.h"

// namespace cbk::platform::metal {

// 	using namespace rendering;

// 	static MTL::VertexFormat shaderDataTypeToMetalFormat(ShaderDataType type) {
// 		switch (type) {
// 			case ShaderDataType::Float:   return MTL::VertexFormatFloat;
// 			case ShaderDataType::Float2:  return MTL::VertexFormatFloat2;
// 			case ShaderDataType::Float3:  return MTL::VertexFormatFloat3;
// 			case ShaderDataType::Float4:  return MTL::VertexFormatFloat4;
// 			case ShaderDataType::Int:     return MTL::VertexFormatInt;
// 			case ShaderDataType::Int2:    return MTL::VertexFormatInt2;
// 			case ShaderDataType::Int3:    return MTL::VertexFormatInt3;
// 			case ShaderDataType::Int4:    return MTL::VertexFormatInt4;
// 			// Mat3/Mat4 need special handling (multiple attribute slots)
// 			default: return MTL::VertexFormatInvalid;
// 		}
// 	}

// 	MetalVertexArray::MetalVertexArray() : m_VertexAttributeIndex(0) {
// 		CBK_PROFILE_FUNCTION();

// 	}

// 	MetalVertexArray::~MetalVertexArray() {
// 		CBK_PROFILE_FUNCTION();

// 		if (m_VertexDescriptor)
// 			m_VertexDescriptor->release();
// 	}

// 	void MetalVertexArray::bind() const { }

// 	void MetalVertexArray::unbind() const { }

// 	void MetalVertexArray::addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
// 		CBK_PROFILE_FUNCTION();

// 		CBK_CORE_ASSERT(vertexBuffer->getLayout().getElements().size(), "Vertex Buffer has no layout!");

// 		if (!m_VertexDescriptor)
// 			m_VertexDescriptor = MTL::VertexDescriptor::vertexDescriptor();

// 		uint32_t bufferIndex = static_cast<uint32_t>(m_VertexBuffers.size());
// 		const auto& layout = vertexBuffer->getLayout();
// 		for (const auto& element : layout) {
// 			auto* attributeDescriptor = m_VertexDescriptor->attributes()->object(m_VertexAttributeIndex);
// 			attributeDescriptor->setFormat(shaderDataTypeToMetalFormat(element.Type));
// 			attributeDescriptor->setOffset(element.Offset);
// 			attributeDescriptor->setBufferIndex(bufferIndex);
// 			m_VertexAttributeIndex++;
// 		}

// 		auto layoutDesc = m_VertexDescriptor->layouts()->object(bufferIndex);
// 		layoutDesc->setStride(layout.getStride());
// 		layoutDesc->setStepRate(1);
// 		layoutDesc->setStepFunction(MTL::VertexStepFunctionPerVertex);

// 		m_VertexBuffers.push_back(vertexBuffer);
// 	}

// 	void MetalVertexArray::setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
// 		CBK_PROFILE_FUNCTION();
// 		m_IndexBuffer = indexBuffer;
// 	}
// }
