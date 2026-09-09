#include <pch.h>
#include "PBRMaterial.h"

#include <Cabrankengine/Renderer/RendererAPI.h>

#ifdef CBK_RENDERER_VULKAN
#include <Platform/Vulkan/VulkanPBRMaterial.h>
#endif

#ifdef CBK_RENDERER_METAL
#include <Platform/Metal/MetalPBRMaterial.h>
#endif

namespace cbk::rendering {

	using namespace scene;

	Ref<PBRMaterial> PBRMaterial::create() {
#ifdef CBK_RENDERER_METAL
		return createRef<platform::metal::MetalPBRMaterial>();
#elif defined(CBK_RENDERER_VULKAN)
		return createRef<platform::vk::VulkanPBRMaterial>();
#else
		CBK_CORE_ASSERT(false, "No renderer API defined!");
		return nullptr;
#endif
	}

	PBRMaterial::PBRMaterial() : Material(ShaderLibrary::get("PBR")), m_AlbedoColor(1.f), m_Metalness(0.f), m_Roughness(0.5f) {
		m_AlbedoMap = DefaultLibrary::getWhiteTexture();
		m_NormalMap = DefaultLibrary::getFlatNormalTexture();
		m_MetalRoughMap = DefaultLibrary::getWhiteTexture();
		m_AOMap = DefaultLibrary::getWhiteTexture();
	}

	void PBRMaterial::setAlbedoMap(const Ref<Texture2D>& tex) {
		m_AlbedoMap = tex;
	}

	void PBRMaterial::setNormalMap(const Ref<Texture2D>& tex) {
		m_NormalMap = tex;
	}

	void PBRMaterial::setMetalRoughMap(const Ref<Texture2D>& tex) {
		m_MetalRoughMap = tex;
	}

	void PBRMaterial::setAOMap(const Ref<Texture2D>& tex) {
		m_AOMap = tex;
	}

	math::Vector3 PBRMaterial::getAlbedoColor() const {
		return m_AlbedoColor;
	}

	float PBRMaterial::getMetalness() const {
		return m_Metalness;
	}

	float PBRMaterial::getRoughness() const {
		return m_Roughness;
	}

	void PBRMaterial::setAlbedoColor(math::Vector3 color) {
		m_AlbedoColor = color;
	}

	void PBRMaterial::setMetalness(float value) {
		m_Metalness = value;
	}

	void PBRMaterial::setRoughness(float value) {
		m_Roughness = value;
	}

	void PBRMaterial::applyTexture(common::TextureType type, const Ref<Texture2D>& texture) {
		switch (type) {
			case common::TextureType::Diffuse:
				setAlbedoMap(texture);
				break;
			case common::TextureType::Normal:
				setNormalMap(texture);
				break;
			case common::TextureType::MetalRoughness:
				setMetalRoughMap(texture);
				break;
			case common::TextureType::AO:
				setAOMap(texture);
				break;
			default:
				break;
		}
	}

	void PBRMaterial::applyProperty(uint32_t key, float value) {
		// Property keys defined in Common/BinaryFormats.h:
		//   2 = Metalness, 3 = Roughness, 4 = BaseColorR, 5 = BaseColorG, 6 = BaseColorB
		// BaseColor arrives one channel at a time; commit on the B channel.
		switch (key) {
			case 2:
				setMetalness(value);
				break;
			case 3:
				setRoughness(value);
				break;
			case 4:
				m_PendingBaseColorR = value;
				break;
			case 5:
				m_PendingBaseColorG = value;
				break;
			case 6:
				setAlbedoColor({ m_PendingBaseColorR, m_PendingBaseColorG, value });
				break;
			default:
				break;
		}
	}

} // namespace cbk::rendering
