#include <pch.h>
#include "OpenGLUniformBuffer.h"

#include <glad/glad.h>

namespace cbk::platform::opengl {

	OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding) : m_RendererID() {
#ifdef CBK_OPENGL_ES
		glGenBuffers(1, &m_RendererID);
		glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
		glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
#else
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
#endif
		glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_RendererID);
	}

	OpenGLUniformBuffer::~OpenGLUniformBuffer() {
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLUniformBuffer::setData(const void* data, uint32_t size, uint32_t offset) {
#ifdef CBK_OPENGL_ES
		glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
		glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
#else
		glNamedBufferSubData(m_RendererID, offset, size, data);
#endif
	}
} // namespace cbk::platform::opengl