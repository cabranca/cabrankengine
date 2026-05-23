#include "RendererAPI.h"

namespace cbk::rendering {
    
#ifdef CBK_RENDERER_OPENGL
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;
#endif

#ifdef CBK_RENDERER_METAL
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::Metal;
#endif

#ifdef CBK_RENDERER_VULKAN
    RendererAPI::API RendererAPI::s_API = RendererAPI::API::Vulkan;
#endif
}
