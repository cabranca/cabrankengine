// #pragma once

// #include <Cabrankengine/Renderer/Buffer.h>

// namespace MTL {
// 	class Buffer;
// }

// namespace cbk::platform::metal {

// 	class MetalVertexBuffer : public rendering::VertexBuffer {
// 		public:
// 			MetalVertexBuffer(uint32_t size);
// 			MetalVertexBuffer(void* vertices, uint32_t size);
// 			~MetalVertexBuffer();

// 			// Binds the vertex buffer so that it can be used in rendering.
// 			virtual void bind() const override;

// 			// Unbinds the vertex buffer so that it is no longer used in rendering.
// 			virtual void unbind() const override;

// 			// Returns the structure of the vertex buffer, which is defined by the BufferLayout.
// 			virtual const rendering::BufferLayout& getLayout() const override { return m_Layout; }
		
// 			// Sets the layout of the vertex buffer, which defines how the vertex attributes are organized.
// 			virtual void setLayout(const rendering::BufferLayout& layout) override { m_Layout = layout; }

// 			virtual void setData(const void* data, uint32_t size) override;

// 			// Returns the Metal buffer for the vertex buffer.
// 			MTL::Buffer* getBuffer() const { return m_Buffer; }

// 		private:
// 			MTL::Buffer* m_Buffer; // Metal buffer for the vertex buffer
// 			uint32_t m_Size; // Size of the vertex buffer
// 			rendering::BufferLayout m_Layout; // Layout of the vertex buffer, defining how the vertex attributes are organized
// 	};

// 	class MetalIndexBuffer : public rendering::IndexBuffer {
// 		public:
// 			MetalIndexBuffer(uint32_t* indices, uint32_t count);
// 			~MetalIndexBuffer();

// 			// Binds the index buffer so that it can be used in rendering.
// 			virtual void bind() const override;

// 			// Unbinds the index buffer so that it is no longer used in rendering.
// 			virtual void unbind() const override;

// 			// Returns the number of indices in the index buffer.
// 			virtual uint32_t getCount() const override { return m_Count; }

// 			// Returns the Metal buffer for the index buffer.
// 			MTL::Buffer* getBuffer() const { return m_Buffer; }

// 		private:
// 			MTL::Buffer* m_Buffer; // Metal buffer for the index buffer
// 			uint32_t m_Count; // Number of indices in the index buffer
// 	};
// }