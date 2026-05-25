#pragma once

#include <Common/Math/Mat4.h>
#include <Cabrankengine/Renderer/Shader.h>

#include <unordered_map>
#include <vector>

// Forward declarations de metal-cpp
namespace MTL { class RenderPipelineState; class Function; class Library; class VertexDescriptor; }

namespace cbk::platform::metal {

	class MetalShader : public rendering::Shader {
		public:
			MetalShader(const std::string& filepath);
			MetalShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
			virtual ~MetalShader();

			// Binds the shader program for use in rendering.
			virtual void bind() const override;

			// Unbinds the shader program, stopping its use in rendering.
			virtual void unbind() const override;

			// Sets an integer value in the shader program.
			void setInt(const std::string& name, int value) override;

			// Sets a int array in the shader program.
			void setIntArray(const std::string& name, uint32_t count, int* values) override;

			// Sets a float value in the shader program.
			void setFloat(const std::string& name, float value) override;

			// Sets a 3 float vector in the shader program.
			void setFloat3(const std::string& name, const math::Vector3& vector) override;

			// Sets a 4 float vector in the shader program.
			void setFloat4(const std::string& name, const math::Vector4& vector) override;

			// Sets a 4x4 float matrix in the shader program.
		    void setMat4(const std::string& name, const math::Mat4& value) override;

			// Returns the name of the shader program given by the user.
			const std::string& getName() const override { return m_Name; }

			// Returns the pipeline state object for this shader.
        	MTL::RenderPipelineState* getPipelineState() const { return m_PipelineState; }

			// Ensures the PSO exists, creating it with the given vertex descriptor if needed.
			void ensurePipelineState(MTL::VertexDescriptor* vertexDescriptor);

			// Returns pending vertex-stage bytes to be set on the encoder before drawing.
			const std::unordered_map<uint32_t, std::vector<uint8_t>>& getPendingVertexBytes() const {
				return m_PendingVertexBytes;
			}

		private:
			// Reads the shader source code from a file.
			std::string readFile(const std::string& filepath);

			// Compiles the Metal shader library from source.
        	void compileLibrary(const std::string& source);

			// Returns the Metal buffer index for a given uniform name.
			// For now this is a simple hardcoded map. TODO: use Metal shader reflection.
			uint32_t getBufferIndex(const std::string& name) const;
			
			std::string m_Name;
			
			MTL::Library* m_Library = nullptr;
			MTL::Function* m_VertexFunction = nullptr;
			MTL::Function* m_FragmentFunction = nullptr;
			MTL::RenderPipelineState* m_PipelineState = nullptr;

			// Pending uniform data to be pushed via setVertexBytes() at draw time.
			// Key: Metal buffer index, Value: raw bytes.
			std::unordered_map<uint32_t, std::vector<uint8_t>> m_PendingVertexBytes;
	};
}
