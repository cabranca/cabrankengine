#include <pch.h>
#include "OpenGLBuffer.h"

#include <glad/glad.h>

namespace cbk::platform::opengl {

	// --------------------- VERTEX BUFFER --------------------- //

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size) : m_RendererId() {
		CBK_PROFILE_FUNCTION();

		glCreateBuffers(1, &m_RendererId);
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(const void* vertices, uint32_t size) : m_RendererId() {
		CBK_PROFILE_FUNCTION();

		glCreateBuffers(1, &m_RendererId); // TODO: research this functions. Cherno thought that "create" elminiated the need of "bind" but it does not.
		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer() {
		CBK_PROFILE_FUNCTION();

		if (m_RendererId)
			glDeleteBuffers(1, &m_RendererId);
	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(OpenGLVertexBuffer&& other) noexcept : m_RendererId(other.m_RendererId) {
		other.m_RendererId = 0;
	}

	OpenGLVertexBuffer& OpenGLVertexBuffer::operator=(OpenGLVertexBuffer&& other) noexcept {
		if (this != &other) {
			if (m_RendererId)
				glDeleteBuffers(1, &m_RendererId);
			m_RendererId = other.m_RendererId;
			other.m_RendererId = 0;
		}
		return *this;
	}

	void OpenGLVertexBuffer::bind() const {
		CBK_PROFILE_FUNCTION();

		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
	}

	void OpenGLVertexBuffer::unbind() const {
		CBK_PROFILE_FUNCTION();

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::setData(const void* data, uint32_t size) {
		CBK_PROFILE_FUNCTION();

		glBindBuffer(GL_ARRAY_BUFFER, m_RendererId);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	// --------------------- INDEX BUFFER --------------------- //

	OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32_t* indices, uint32_t count) : m_RendererId(), m_Count(count) {
		CBK_PROFILE_FUNCTION();

		glCreateBuffers(1, &m_RendererId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer() {
		CBK_PROFILE_FUNCTION();

		if (m_RendererId)
			glDeleteBuffers(1, &m_RendererId);
	}

	OpenGLIndexBuffer::OpenGLIndexBuffer(OpenGLIndexBuffer&& other) noexcept
	    : m_RendererId(other.m_RendererId), m_Count(other.m_Count) {
		other.m_RendererId = 0;
		other.m_Count = 0;
	}

	OpenGLIndexBuffer& OpenGLIndexBuffer::operator=(OpenGLIndexBuffer&& other) noexcept {
		if (this != &other) {
			if (m_RendererId)
				glDeleteBuffers(1, &m_RendererId);
			m_RendererId = other.m_RendererId;
			m_Count = other.m_Count;
			other.m_RendererId = 0;
			other.m_Count = 0;
		}
		return *this;
	}

	void OpenGLIndexBuffer::bind() const {
		CBK_PROFILE_FUNCTION();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererId);
	}

	void OpenGLIndexBuffer::unbind() const {
		CBK_PROFILE_FUNCTION();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}
