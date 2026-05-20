#pragma once

#include <Cabrankengine/Math/Mat4.h>

#include "Material.h"

namespace cbk::rendering {

	// Abstract 32-sampler batch material backing the TextRenderer glyph pipeline.
	// Mirrors Texture2DMaterial; the difference is the shader (glyphs sample 1:1).
	class TextMaterial : public Material {
	  public:
		static Ref<TextMaterial> create();

		virtual void setTextureSlot(uint32_t slot, const Ref<Texture2D>& texture) = 0;
		virtual void setViewProjection(const math::Mat4& viewProjection) = 0;

	  protected:
		TextMaterial() : Material(ShaderLibrary::get("Text")) {}
	};

} // namespace cbk::rendering
