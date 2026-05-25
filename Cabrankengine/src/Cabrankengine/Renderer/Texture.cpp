#include <pch.h>
#include "Texture.h"

#ifdef CBK_RENDERER_METAL
	#include <Platform/Metal/MetalTexture.h>
#endif

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLTexture.h>
#endif

#ifdef CBK_RENDERER_VULKAN
    #include <Platform/Vulkan/VulkanTexture.h>
#endif

namespace cbk::rendering {

	Ref<Texture2D> Texture2D::create(const TextureSpecification& specification) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLTexture2D>(specification);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalTexture2D>(specification);
#elif defined(CBK_RENDERER_VULKAN)
        return createRef<platform::vk::VulkanTexture>(specification);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	Ref<Texture2D> Texture2D::create(const std::string& path) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLTexture2D>(path);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalTexture2D>(path);
#elif defined(CBK_RENDERER_VULKAN)
        return createRef<platform::vk::VulkanTexture>(path);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	Ref<Texture2D> Texture2D::create(const FT_Face& face) {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLTexture2D>(face);
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalTexture2D>(face);
#elif defined(CBK_RENDERER_VULKAN)
        return createRef<platform::vk::VulkanTexture>(face);
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}
} // namespace cbk::rendering
