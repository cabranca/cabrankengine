#pragma once

#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>
#include <Common/Math/Mat4.h>

#include "IMetalRecordable.h"

namespace MTL {
	class RenderPipelineState;
	class RenderCommandEncoder;
	class DepthStencilState;
} // namespace MTL

namespace cbk::platform::metal {

	// Lit material for Cook-Torrance PBR models. Owns a shared pipeline (from the
	// "PBR" shader); per-instance state (albedo/normal/metalRough/AO maps + albedo
	// color, metalness, roughness) lives on the rendering::PBRMaterial base. Scene
	// globals come from the Renderer and the point-light array from the Renderer's
	// light storage buffer; both are pushed as bytes in record(). Mirrors
	// VulkanPBRMaterial.
	class MetalPBRMaterial : public rendering::PBRMaterial, public IMetalRecordable {
	  public:
		MetalPBRMaterial();
		~MetalPBRMaterial() override;

		void bind() const override {}

		// IMetalRecordable
		void record(MTL::RenderCommandEncoder* encoder, const math::Mat4& transform) const override;

		// Per-class pipeline cleanup. Called by MetalRendererAPI::shutdown().
		static void destroySharedResources();

	  private:
		// Fragment push block. Layout matches the PBR.metal PushData (packed_float3 +
		// two floats = 20 bytes), which mirrors PBR.slang's albedoColor/metalness/roughness.
		struct PushData {
			math::Vector3 albedoColor;
			float metalness;
			float roughness;
		};

		static void initSharedResourcesIfNeeded();

		static bool s_Initialized;
		static MTL::RenderPipelineState* s_Pipeline;
		// Opaque 3D: depth test LessEqual + depth write enabled.
		static MTL::DepthStencilState* s_DepthState;
	};

} // namespace cbk::platform::metal
