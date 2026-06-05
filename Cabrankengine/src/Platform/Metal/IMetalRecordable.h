#pragma once

#include <Common/Math/Mat4.h>

namespace MTL {
	class RenderCommandEncoder;
}

namespace cbk::platform::metal {

	// Capability interface for materials that know how to record themselves into a
	// Metal render command encoder. Lives as a sibling of rendering::Material —
	// concrete Metal materials multi-inherit from both, and MetalRendererAPI
	// dynamic_casts to this interface when issuing draws.
	class IMetalRecordable {
	  public:
		virtual ~IMetalRecordable() = default;

		// Records everything needed to draw with this material into the encoder: sets
		// the render pipeline state, pushes per-frame/per-draw uniform bytes (scene
		// globals, model matrix, material parameters) and binds the material's
		// textures. Called by MetalRendererAPI after the render-pass encoder is
		// created and before drawIndexedPrimitives. transform is the per-draw model
		// matrix; materials that batch in world space (Text, Texture2D) ignore it.
		//
		// The buffer-index convention is centralised in MetalBinding.h so it is not
		// duplicated across concrete materials.
		virtual void record(MTL::RenderCommandEncoder* encoder, const math::Mat4& transform) const = 0;
	};

} // namespace cbk::platform::metal
