#pragma once

#include <Common/Math/Mat4.h>

namespace cbk::rendering {

	// A Shader is a program that runs on the GPU and is used to render graphics.
	class Shader {
		public:
			virtual ~Shader() = default;

			// Binds the shader program for use in rendering.
			virtual void bind() const = 0;

			// Unbinds the shader program, stopping its use in rendering.
			virtual void unbind() const = 0;

			// Sets a int value in the shader program.
			virtual void setInt(const std::string& name, int value) = 0;

			// Sets a int array in the shader program.
			virtual void setIntArray(const std::string& name, uint32_t count, int* values) = 0;

			// Sets a float value in the shader program.
			virtual void setFloat(const std::string& name, float value) = 0;

			// Sets a 3 float vector in the shader program.
			virtual void setFloat3(const std::string& name, const math::Vector3& vector) = 0;

			// Sets a 4 float vector in the shader program.
			virtual void setFloat4(const std::string& name, const math::Vector4& vector) = 0;

			// Sets a 4x4 float matrix in the shader program.
		    virtual void setMat4(const std::string& name, const math::Mat4& value) = 0;

			// Returns the name of the shader program given by the user.
			virtual const std::string& getName() const = 0;

			// Creates and returns a shader from a file path.
			static Ref<Shader> create(const std::string& filepath);

			// Creates and returns a shader from vertex and fragment shader source code.
			static Ref<Shader> create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	};

	// ShaderLibrary is a class that manages a collection of shaders.
	class ShaderLibrary {
		public:
			// Adds a shader to the library with a specified name.
			static void add(const std::string& name, const Ref<Shader>& shader);

			// Adds a shader to the library without a specified name.
			static void add(const Ref<Shader>& shader);

			// Loads a shader from a file path and adds it to the library with the name derived from the file path .
			static Ref<Shader> load(const std::string& filepath);

			// Loads a shader from a file path and adds it to the library with a specified name.
			static Ref<Shader> load(const std::string& name, const std::string& filepath);

			// Retrieves a shader from the library by its name.
			static Ref<Shader> get(const std::string& name);

			// Release all cached shaders. Call from Renderer::shutdown so the Refs
			// destruct while the Vulkan device is still alive.
			static void shutdown() { s_Shaders.clear(); }

		private:
			static inline std::unordered_map<std::string, Ref<Shader>> s_Shaders; // A map from the shader names to the shader references.
	};
}
