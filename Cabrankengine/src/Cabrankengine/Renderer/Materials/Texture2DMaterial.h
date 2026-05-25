#pragma once

#include <Common/Math/Mat4.h>

#include "Material.h"

namespace cbk::rendering {

	// Abstract 32-sampler batch material backing the Renderer2D quad pipeline.
	// Concrete backends implement slot population, view-projection delivery and
	// binding; engine code only ever sees this interface.
	class Texture2DMaterial : public Material {
	  public:
		static Ref<Texture2DMaterial> create();

		virtual void setTextureSlot(uint32_t slot, const Ref<Texture2D>& texture) = 0;
		virtual void setViewProjection(const math::Mat4& viewProjection) = 0;

	  protected:
		Texture2DMaterial() : Material(ShaderLibrary::get("Texture")) {}
	};

} // namespace cbk::rendering
