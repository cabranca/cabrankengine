#include <pch.h>

#include "TextMaterial.h"

#include <Cabrankengine/Renderer/RendererAPI.h>

#ifdef CBK_RENDERER_OPENGL
	#include <Platform/OpenGL/OpenGLTextMaterial.h>
#endif

#ifdef CBK_RENDERER_VULKAN
	#include <Platform/Vulkan/VulkanTextMaterial.h>
#endif

namespace cbk::rendering {

	Ref<TextMaterial> TextMaterial::create() {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLTextMaterial>();
#elif defined(CBK_RENDERER_VULKAN)
		return createRef<platform::vk::VulkanTextMaterial>();
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

} // namespace cbk::rendering
