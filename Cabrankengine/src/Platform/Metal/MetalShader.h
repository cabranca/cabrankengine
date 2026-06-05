#pragma once

#include <Common/Math/Mat4.h>
#include <Cabrankengine/Renderer/Shader.h>

// Forward declarations de metal-cpp
namespace MTL {
	class Function;
	class Library;
} // namespace MTL

namespace cbk::platform::metal {

	// Minimal shader resource: a compiled MTL::Library plus its vertex/fragment
	// entry-point functions. Pipeline state (vertex descriptor, color/depth formats,
	// blend state) lives on the concrete Metal material that uses this shader —
	// mirroring VulkanShader, where the SPIR-V module is the shader and the pipeline
	// belongs to the material.
	class MetalShader : public rendering::Shader {
	  public:
		MetalShader(const std::string& filepath);
		MetalShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
		~MetalShader() override;

		// Per-draw data flows through the material's record() (setVertexBytes /
		// setFragmentBytes / setFragmentTexture), so bind() and the uniform setters
		// are no-ops here, exactly as on Vulkan.
		void bind() const override {}
		void unbind() const override {}

		void setInt(const std::string&, int) override {}
		void setIntArray(const std::string&, uint32_t, int*) override {}
		void setFloat(const std::string&, float) override {}
		void setFloat3(const std::string&, const math::Vector3&) override {}
		void setFloat4(const std::string&, const math::Vector4&) override {}
		void setMat4(const std::string&, const math::Mat4&) override {}

		const std::string& getName() const override {
			return m_Name;
		}

		// Entry-point functions used by materials to build their pipeline state.
		[[nodiscard]] MTL::Function* getVertexFunction() const {
			return m_VertexFunction;
		}
		[[nodiscard]] MTL::Function* getFragmentFunction() const {
			return m_FragmentFunction;
		}
		[[nodiscard]] MTL::Library* getLibrary() const {
			return m_Library;
		}

	  private:
		// Reads the shader source code from a file.
		std::string readFile(const std::string& filepath);

		// Compiles the Metal library from source and resolves vertex_main/fragment_main.
		void compileLibrary(const std::string& source);

		std::string m_Name;

		MTL::Library* m_Library = nullptr;
		MTL::Function* m_VertexFunction = nullptr;
		MTL::Function* m_FragmentFunction = nullptr;
	};
} // namespace cbk::platform::metal
