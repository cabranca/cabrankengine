#include <pch.h>

#include "PhongMaterial.h"

#include <Cabrankengine/Renderer/RendererAPI.h>

#ifdef CBK_RENDERER_OPENGL
#include <Platform/OpenGL/OpenGLPhongMaterial.h>
#endif

#ifdef CBK_RENDERER_VULKAN
#include <Platform/Vulkan/VulkanPhongMaterial.h>
#endif

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalPhongMaterial.h>
#endif

namespace cbk::rendering {

	Ref<PhongMaterial> PhongMaterial::create() {
#ifdef CBK_RENDERER_OPENGL
		return createRef<platform::opengl::OpenGLPhongMaterial>();
#elif defined(CBK_RENDERER_METAL)
		return createRef<platform::metal::MetalPhongMaterial>();
#elif defined(CBK_RENDERER_VULKAN)
		return createRef<platform::vk::VulkanPhongMaterial>();
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	PhongMaterial::PhongMaterial() : Material(ShaderLibrary::get("Phong")) {
		// Default texture slots so Vulkan descriptor sets can be written immediately
		// even before a model loader supplies real maps.
		m_DiffuseMap = scene::DefaultLibrary::getWhiteTexture();
		m_SpecularMap = scene::DefaultLibrary::getWhiteTexture();
	}

	void PhongMaterial::setDiffuseMap(const Ref<Texture2D>& texture) {
		m_DiffuseMap = texture;
	}

	void PhongMaterial::setSpecularMap(const Ref<Texture2D>& texture) {
		m_SpecularMap = texture;
	}

	void PhongMaterial::setShininess(float shininess) {
		m_Shininess = shininess;
	}

	void PhongMaterial::applyTexture(common::TextureType type, const Ref<Texture2D>& texture) {
		switch (type) {
			case common::TextureType::Diffuse:
				setDiffuseMap(texture);
				break;
			case common::TextureType::Specular:
				setSpecularMap(texture);
				break;
			default:
				break;
		}
	}

	void PhongMaterial::applyProperty(uint32_t key, float value) {
		if (key == 1)
			setShininess(value); // 1 = Shininess (see Common/BinaryFormats.h property keys)
	}

} // namespace cbk::rendering
