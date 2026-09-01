#include <pch.h>

#include "Texture2DMaterial.h"

#include <Cabrankengine/Renderer/RendererAPI.h>

#ifdef CBK_RENDERER_OPENGL
#include <Platform/OpenGL/OpenGLTexture2DMaterial.h>
#endif

#ifdef CBK_RENDERER_VULKAN
#include <Platform/Vulkan/VulkanTexture2DMaterial.h>
#endif

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalTexture2DMaterial.h>
#endif

namespace cbk::rendering {

	Ref<Texture2DMaterial> Texture2DMaterial::create() {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLTexture2DMaterial>();
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalTexture2DMaterial>();
#elif defined(CBK_RENDERER_VULKAN)
		return nullptr;
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

} // namespace cbk::rendering
