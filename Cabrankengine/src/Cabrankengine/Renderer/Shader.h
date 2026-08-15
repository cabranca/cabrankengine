#pragma once

#include <Common/Math/Mat4.h>

namespace cbk::rendering {

	// A Shader is a program that runs on the GPU and is used to render graphics.
	class Shader {
	  public:
		virtual ~Shader() = default;

		// Returns the name of the shader program given by the user.
		[[nodiscard]] virtual const std::string& getName() const = 0;

		// Creates and returns a shader from a file path.
		[[nodiscard]] static Ref<Shader> create(const std::string& filepath);
	};

	// ShaderLibrary is a class that manages a collection of shaders.
	class ShaderLibrary {
	  public:
		// Adds a shader to the library with a specified name.
		static void add(const std::string& name, const Ref<Shader>& shader);

		// Adds a shader to the library without a specified name.
		static void add(const Ref<Shader>& shader);

		// Loads a shader from a file path and adds it to the library with the name derived from the file path .
		static void load(const std::string& filepath);

		// Loads a shader from a file path and adds it to the library with a specified name.
		static void load(const std::string& name, const std::string& filepath);

		// Retrieves a shader from the library by its name.
		[[nodiscard]] static Ref<Shader> get(const std::string& name);

		// Release all cached shaders.
		static void shutdown() {
			s_Shaders.clear();
		}

	  private:
		static inline std::unordered_map<std::string, Ref<Shader>> s_Shaders; // A map from the shader names to the shader references.
	};
} // namespace cbk::rendering
