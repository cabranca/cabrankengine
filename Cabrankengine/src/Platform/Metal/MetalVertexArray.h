// #pragma once

// #include <Cabrankengine/Renderer/VertexArray.h>

// namespace MTL {
// 	class VertexDescriptor;
// }

// namespace cbk::platform::metal {

// 	class MetalVertexArray : public rendering::VertexArray {
// 		public:
// 			MetalVertexArray();
// 			~MetalVertexArray();

// 			// Binds the vertex array for use in rendering.
// 			virtual void bind() const override;

// 			// Unbinds the vertex array, stopping its use in rendering.
// 			virtual void unbind() const override;

// 			// Adds a vertex buffer to the vertex array.
// 			virtual void addVertexBuffer(const Ref<rendering::VertexBuffer>& vertexBuffer) override;
			
// 			// Sets the index buffer for the vertex array.
// 			virtual void setIndexBuffer(const Ref<rendering::IndexBuffer>& indexBuffer) override;

// 			// Returns the vertex buffers in the vertex array.
// 			virtual const std::vector<Ref<rendering::VertexBuffer>>& getVertexBuffers() const override { return m_VertexBuffers; }
			
// 			// Returns the index buffer in the vertex array.
// 			virtual const Ref<rendering::IndexBuffer>& getIndexBuffer() const override { return m_IndexBuffer; }

// 			// Returns the vertex descriptor for the vertex array.
// 			MTL::VertexDescriptor* getVertexDescriptor() const { return m_VertexDescriptor; }

// 		private:
// 			std::vector<Ref<rendering::VertexBuffer>> m_VertexBuffers; // Vector of vertex buffers in the vertex array
// 			Ref<rendering::IndexBuffer> m_IndexBuffer; // Index buffer in the vertex array
// 			MTL::VertexDescriptor* m_VertexDescriptor; // Vertex descriptor for the vertex array
// 			uint32_t m_VertexAttributeIndex; // Index of the vertex attribute in the vertex descriptor
// 	};
// }