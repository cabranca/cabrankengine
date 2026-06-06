#pragma once

#include "Material.h"

namespace cbk::rendering {

	// Abstract: data + behavior live here; bind() is implemented per-backend.
	class PBRMaterial : public Material {
	  public:
		[[nodiscard]] static Ref<PBRMaterial> create();

		void setAlbedoMap(const Ref<Texture2D>& tex);
		void setNormalMap(const Ref<Texture2D>& tex);
		void setMetalRoughMap(const Ref<Texture2D>& tex);
		void setAOMap(const Ref<Texture2D>& tex);

		[[nodiscard]] math::Vector3 getAlbedoColor() const;
		[[nodiscard]] float getMetalness() const;
		[[nodiscard]] float getRoughness() const;

		void setAlbedoColor(math::Vector3 color);
		void setMetalness(float value);
		void setRoughness(float value);

		void applyTexture(common::TextureType type, const Ref<Texture2D>& texture) override;
		void applyProperty(uint32_t key, float value) override;

		[[nodiscard]] Ref<Material> instantiate() const override {
			return create();
		}

	  protected:
		PBRMaterial();

		Ref<Texture2D> m_AlbedoMap;
		Ref<Texture2D> m_NormalMap;
		Ref<Texture2D> m_MetalRoughMap;
		Ref<Texture2D> m_AOMap;

		math::Vector3 m_AlbedoColor;
		float m_Metalness;
		float m_Roughness;

		// Channel scratch for incremental BaseColorR/G/B properties (keys 4/5/6).
		float m_PendingBaseColorR = 1.f;
		float m_PendingBaseColorG = 1.f;
	};
} // namespace cbk::rendering