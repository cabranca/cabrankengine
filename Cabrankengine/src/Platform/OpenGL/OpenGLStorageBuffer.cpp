#include <pch.h>

#ifndef CBK_OPENGL_ES

#include "OpenGLStorageBuffer.h"

#include <glad/glad.h>

namespace cbk::platform::opengl {

	OpenGLStorageBuffer::OpenGLStorageBuffer(uint32_t size) {
		glCreateBuffers(1, &m_RendererID);
		glNamedBufferData(m_RendererID, size, nullptr, GL_DYNAMIC_DRAW);
	}

	OpenGLStorageBuffer::~OpenGLStorageBuffer() {
		glDeleteBuffers(1, &m_RendererID);
	}

	void OpenGLStorageBuffer::setData(const void* data, uint32_t size) {
		glNamedBufferSubData(m_RendererID, 0, size, data);
	}

	void OpenGLStorageBuffer::bind(uint32_t bindingPoint) const {
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_RendererID);
	}
} // namespace cbk::platform::opengl

#endif // !CBK_OPENGL_ES
