#pragma once

#include <Cabrankengine/Renderer/GeometryDescriptor.h>

#include "OpenGLBuffer.h"

namespace cbk::platform::opengl {

	class OpenGLVertexArray : public rendering::GeometryDescriptor {
	  public:
		OpenGLVertexArray(const void* vertexData, size_t vertexDataSize, const void* indexData, size_t indexDataSize,
		                  const rendering::VertexLayout& layout);
		OpenGLVertexArray(size_t vertexDataSize, const void* indexData, size_t indexDataSize, const rendering::VertexLayout& layout);
		~OpenGLVertexArray();

		// Binds the vertex array for use in rendering.
		virtual void bind() const;

		// Unbinds the vertex array, stopping its use in rendering.
		virtual void unbind() const;

		void setData(const void* data, uint32_t size) override;

		uint32_t getIndexCount() const override;

	  private:
		std::vector<OpenGLVertexBuffer> m_VertexBuffers; // Vector of vertex buffers in the vertex array
		OpenGLIndexBuffer m_IndexBuffer;                 // Index buffer in the vertex array

		uint32_t m_RendererId; // Renderer ID for the OpenGL vertex array, used to identify it in the OpenGL context
	};
} // namespace cbk::platform::opengl