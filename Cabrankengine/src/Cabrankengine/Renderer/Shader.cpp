#include <pch.h>
#include "Shader.h"

#ifdef CBK_RENDERER_METAL
	#include <Platform/Metal/MetalShader.h>
#endif

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLShader.h>
#endif

#ifdef CBK_RENDERER_VULKAN
    #include <Platform/Vulkan/VulkanShader.h>
#endif

namespace cbk::rendering {

	Ref<Shader> Shader::create(const std::string& filepath) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLShader>(filepath);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalShader>(filepath);
#elif defined(CBK_RENDERER_VULKAN)
        return createRef<platform::vk::VulkanShader>(filepath);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	Ref<Shader> Shader::create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLShader>(name, vertexSrc, fragmentSrc);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalShader>(name, vertexSrc, fragmentSrc);
#elif defined(CBK_RENDERER_VULKAN)
        return createRef<platform::vk::VulkanShader>(name, vertexSrc, fragmentSrc);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	void ShaderLibrary::add(const std::string& name, const Ref<Shader>& shader) {
		CBK_CORE_ASSERT(!s_Shaders.contains(name), "Shader already exists!");
		s_Shaders[name] = shader;
	}

	void ShaderLibrary::add(const Ref<Shader>& shader) {
		const auto& name = shader->getName();
		CBK_CORE_ASSERT(!s_Shaders.contains(name), "Shader already exists!");
		s_Shaders[name] = shader;
	}

	Ref<Shader> ShaderLibrary::load(const std::string& filepath) {
		auto shader = Shader::create(filepath);
		add(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::load(const std::string& name, const std::string& filepath) {
		auto shader = Shader::create(filepath);
		add(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::get(const std::string& name) {
		CBK_CORE_ASSERT(s_Shaders.contains(name), "Shader not found!");
		return s_Shaders[name];
	}
}

