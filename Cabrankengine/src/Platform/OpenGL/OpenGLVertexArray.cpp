#include <pch.h>
#include "OpenGLVertexArray.h"

#include <glad/glad.h>

namespace cbk::platform::opengl {

	using namespace rendering;

	static GLenum ShaderDataType2OpenGLBaseType(ShaderDataType type) {
		switch (type) {
			case ShaderDataType::Float:
				return GL_FLOAT;
			case ShaderDataType::Float2:
				return GL_FLOAT;
			case ShaderDataType::Float3:
				return GL_FLOAT;
			case ShaderDataType::Float4:
				return GL_FLOAT;
			case ShaderDataType::Mat3:
				return GL_FLOAT;
			case ShaderDataType::Mat4:
				return GL_FLOAT;
			case ShaderDataType::Int:
				return GL_INT;
			case ShaderDataType::Int2:
				return GL_INT;
			case ShaderDataType::Int3:
				return GL_INT;
			case ShaderDataType::Int4:
				return GL_INT;
			case ShaderDataType::Bool:
				return GL_BOOL;
		}

		CBK_CORE_ASSERT(false, "Unknown Shader Type!");
		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray(const void* vertexData, size_t vertexDataSize, const void* indexData, size_t indexDataSize,
	                                     const VertexLayout& layout)
	    : m_RendererId(), m_IndexBuffer(static_cast<const uint32_t*>(indexData), indexDataSize / sizeof(uint32_t)) {
		CBK_PROFILE_FUNCTION();

#ifdef CBK_OPENGL_ES
		glGenVertexArrays(1, &m_RendererId);
#else
		glCreateVertexArrays(1, &m_RendererId);
#endif

		CBK_CORE_ASSERT(layout.getElements().size(), "Vertex Buffer has no layout!");
		OpenGLVertexBuffer vbo{ vertexData, static_cast<uint32_t>(
			                                    vertexDataSize) }; // the glad call exepcts signed long int. Not sure about this conversion.
		glBindVertexArray(m_RendererId);
		vbo.bind();

		uint32_t index = 0;
		for (const auto& element: layout) {
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, element.getComponentCount(), ShaderDataType2OpenGLBaseType(element.Type),
			                      element.Normalized ? GL_TRUE : GL_FALSE, layout.getStride(),
			                      reinterpret_cast<const void*>(static_cast<uintptr_t>(element.Offset)));
			index++;
		}

		m_VertexBuffers.push_back(std::move(vbo));
		m_IndexBuffer.bind();
	}

	OpenGLVertexArray::OpenGLVertexArray(size_t vertexDataSize, const void* indexData, size_t indexDataSize, const VertexLayout& layout)
	    : m_RendererId(), m_IndexBuffer(static_cast<const uint32_t*>(indexData), indexDataSize / sizeof(uint32_t)) {
		CBK_PROFILE_FUNCTION();

#ifdef CBK_OPENGL_ES
		glGenVertexArrays(1, &m_RendererId);
#else
		glCreateVertexArrays(1, &m_RendererId);
#endif

		CBK_CORE_ASSERT(layout.getElements().size(), "Vertex Buffer has no layout!");
		OpenGLVertexBuffer vbo{ static_cast<uint32_t>(
			vertexDataSize) }; // the glad call exepcts signed long int. Not sure about this conversion.
		glBindVertexArray(m_RendererId);
		vbo.bind();

		uint32_t index = 0;
		for (const auto& element: layout) {
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index, element.getComponentCount(), ShaderDataType2OpenGLBaseType(element.Type),
			                      element.Normalized ? GL_TRUE : GL_FALSE, layout.getStride(),
			                      reinterpret_cast<const void*>(static_cast<uintptr_t>(element.Offset)));
			index++;
		}

		m_VertexBuffers.push_back(std::move(vbo));
		m_IndexBuffer.bind();
	}

	OpenGLVertexArray::~OpenGLVertexArray() {
		CBK_PROFILE_FUNCTION();

		glDeleteVertexArrays(1, &m_RendererId);
	}

	void OpenGLVertexArray::bind() const {
		CBK_PROFILE_FUNCTION();

		glBindVertexArray(m_RendererId);
	}

	void OpenGLVertexArray::unbind() const {
		CBK_PROFILE_FUNCTION();
		
		glBindVertexArray(0);
	}

	void OpenGLVertexArray::setData(const void* data, uint32_t size) {
		m_VertexBuffers[0].setData(data, size);
	}

	uint32_t OpenGLVertexArray::getIndexCount() const {
		return m_IndexBuffer.getCount();
	}
} // namespace cbk::platform::opengl
