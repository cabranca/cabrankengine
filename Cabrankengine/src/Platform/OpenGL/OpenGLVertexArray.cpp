#include <pch.h>
#include "OpenGLVertexArray.h"

#include <glad/glad.h>

#include <Cabrankengine/Renderer/Buffer.h>

namespace cbk::platform::opengl {

	using namespace rendering;

	static GLenum ShaderDataType2OpenGLBaseType(ShaderDataType type) {
		switch (type) {
			case ShaderDataType::Float:   return GL_FLOAT;
			case ShaderDataType::Float2:  return GL_FLOAT;
			case ShaderDataType::Float3:  return GL_FLOAT;
			case ShaderDataType::Float4:  return GL_FLOAT;
			case ShaderDataType::Mat3:    return GL_FLOAT;
			case ShaderDataType::Mat4:    return GL_FLOAT;
			case ShaderDataType::Int:     return GL_INT;
			case ShaderDataType::Int2:    return GL_INT;
			case ShaderDataType::Int3:    return GL_INT;
			case ShaderDataType::Int4:    return GL_INT;
			case ShaderDataType::Bool:    return GL_BOOL;
		}

		CBK_CORE_ASSERT(false, "Unknown Shader Type!");
		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray() : m_RendererId() {
		CBK_PROFILE_FUNCTION();

#ifdef CBK_OPENGL_ES
		glGenVertexArrays(1, &m_RendererId);
#else
		glCreateVertexArrays(1, &m_RendererId);
#endif
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

	void OpenGLVertexArray::addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
		CBK_PROFILE_FUNCTION();

		CBK_CORE_ASSERT(vertexBuffer->getLayout().getElements().size(), "Vertex Buffer has no layout!");

		glBindVertexArray(m_RendererId);
		vertexBuffer->bind();

		const auto& layout = vertexBuffer->getLayout();
		uint32_t index = 0;
		for (const auto& element : layout) {
			glEnableVertexAttribArray(index);
			glVertexAttribPointer(index,
				element.getComponentCount(),
				ShaderDataType2OpenGLBaseType(element.Type),
				element.Normalized ? GL_TRUE : GL_FALSE,
				layout.getStride(),
				reinterpret_cast<const void*>(static_cast<uintptr_t>(element.Offset)));
			index++;
		}

		m_VertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArray::setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
		CBK_PROFILE_FUNCTION();

		glBindVertexArray(m_RendererId);
		indexBuffer->bind();

		m_IndexBuffer = indexBuffer;
	}
}
