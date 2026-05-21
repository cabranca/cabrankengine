#pragma once

#include "Material.h"

namespace cbk::rendering {

	// Abstract: data + behavior live here; bind() is implemented per-backend.
	class PhongMaterial : public Material {
	  public:
		static Ref<PhongMaterial> create();

		void setDiffuseMap(const Ref<Texture2D>& texture);
		void setSpecularMap(const Ref<Texture2D>& texture);
		void setShininess(float shininess);

		void applyTexture(common::TextureType type, const Ref<Texture2D>& texture) override;
		void applyProperty(uint32_t key, float value) override;

		Ref<Material> instantiate() const override { return create(); }

	  protected:
		PhongMaterial();

		Ref<Texture2D> m_DiffuseMap;
		Ref<Texture2D> m_SpecularMap;
		float m_Shininess = 32.0f;
	};
} // namespace cbk::rendering