#pragma once

#include <Cabrankengine/Renderer/Materials/PhongMaterial.h>
#include <Common/Math/Mat4.h>

#include "IMetalRecordable.h"

namespace MTL {
	class RenderPipelineState;
	class RenderCommandEncoder;
	class DepthStencilState;
} // namespace MTL

namespace cbk::platform::metal {

	// Lit material for Phong-shaded models. Owns a shared pipeline (from the "Phong"
	// shader); per-instance state (diffuse/specular maps, shininess) lives on the
	// rendering::PhongMaterial base. Scene globals are pulled from the Renderer and
	// pushed as bytes in record(). Mirrors VulkanPhongMaterial.
	class MetalPhongMaterial : public rendering::PhongMaterial, public IMetalRecordable {
	  public:
		MetalPhongMaterial();
		~MetalPhongMaterial() override;

		void bind() const override {}

		// IMetalRecordable
		void record(MTL::RenderCommandEncoder* encoder, const math::Mat4& transform) const override;

		// Per-class pipeline cleanup. Called by MetalRendererAPI::shutdown().
		static void destroySharedResources();

	  private:
		static void initSharedResourcesIfNeeded();

		static bool s_Initialized;
		static MTL::RenderPipelineState* s_Pipeline;
		// Opaque 3D: depth test LessEqual + depth write enabled.
		static MTL::DepthStencilState* s_DepthState;
	};

} // namespace cbk::platform::metal
