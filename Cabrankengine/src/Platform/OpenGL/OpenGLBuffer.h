#pragma once

namespace cbk::platform::opengl {

	class OpenGLVertexBuffer {
	  public:
		OpenGLVertexBuffer(uint32_t size);
		OpenGLVertexBuffer(const void* vertices, uint32_t size);
		~OpenGLVertexBuffer();

		OpenGLVertexBuffer(const OpenGLVertexBuffer&) = delete;
		OpenGLVertexBuffer& operator=(const OpenGLVertexBuffer&) = delete;
		OpenGLVertexBuffer(OpenGLVertexBuffer&& other) noexcept;
		OpenGLVertexBuffer& operator=(OpenGLVertexBuffer&& other) noexcept;

		// Binds the vertex buffer so that it can be used in rendering.
		void bind() const;

		// Unbinds the vertex buffer so that it is no longer used in rendering.
		void unbind() const;

		void setData(const void* data, uint32_t size);

	  private:
		uint32_t m_RendererId; // OpenGL renderer ID for the vertex buffer
	};

	class OpenGLIndexBuffer {
	  public:
		OpenGLIndexBuffer(const uint32_t* indices, uint32_t count);
		~OpenGLIndexBuffer();

		OpenGLIndexBuffer(const OpenGLIndexBuffer&) = delete;
		OpenGLIndexBuffer& operator=(const OpenGLIndexBuffer&) = delete;
		OpenGLIndexBuffer(OpenGLIndexBuffer&& other) noexcept;
		OpenGLIndexBuffer& operator=(OpenGLIndexBuffer&& other) noexcept;

		// Binds the index buffer so that it can be used in rendering.
		void bind() const;

		// Unbinds the index buffer so that it is no longer used in rendering.
		void unbind() const;

		// Returns the number of indices in the index buffer.
		uint32_t getCount() const {
			return m_Count;
		}

	  private:
		uint32_t m_RendererId; // OpenGL renderer ID for the index buffer
		uint32_t m_Count;      // Number of indices in the index buffer
	};
} // namespace cbk::platform::opengl