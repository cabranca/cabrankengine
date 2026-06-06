#pragma once

#include <Common/Math/Mat4.h>

#include "Lights.h"
#include "RendererAPI.h"

namespace cbk::rendering {

	// Forward declarations
	class Material;
	class Shader;
	class StorageBuffer;
	class UniformBuffer;
	class GeometryDescriptor;

	// This is an abstracted API for rendering graphics in a game engine.
	class Renderer {
	  public:
		// Initializes the renderer, setting up necessary resources and state.
		static void init();

		// Shuts down the renderer, releasing all resources and cleaning up state.
		static void shutdown();

		// Sets the necessary general data to render a scene, such as the camera, the lighting, etc.
		static void beginScene(const math::Mat4& viewProjectionMatrix, const math::Vector3& cameraWorldPosition,
		                       const LightEnvironment& environment);

		// Ends the current scene, finalizing rendering operations.
		static void endScene();

		// Submits a draw call to render a shader with a vertex array and an optional transformation matrix.
		static void submit(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform);

		// Sets the viewport dimensions for rendering.
		static void onWindowResize(uint32_t width, uint32_t height);

		// Returns the current API being used, allowing access to lower-level rendering functions.
		[[nodiscard]] static RendererAPI::API getAPI() {
			return RendererAPI::getAPI();
		}

		// Per-frame scene globals UBO (view-projection, dir light, camera pos).
		// Backend materials need its descriptor to wire pipelines on Vulkan.
		[[nodiscard]] static Ref<UniformBuffer> getSceneUBO();

		// Point-light SSBO uploaded each beginScene(). Exposed for the same
		// reason as getSceneUBO(): VulkanPBRMaterial pulls its descriptor set
		// layout when building the pipeline layout, and VulkanRendererAPI binds
		// the per-frame descriptor set at draw time.
		[[nodiscard]] static Ref<StorageBuffer> getLightSSBO();

		// Raw scene-globals block (view-projection, dir light, camera pos) — the
		// same bytes uploaded to the scene UBO each beginScene(). Exposed for
		// backends that push the block as bytes instead of binding a UBO (Metal,
		// which has no UniformBuffer implementation).
		[[nodiscard]] static const void* sceneUniformData();
		[[nodiscard]] static uint32_t sceneUniformSize();

	  private:
		// This structure holds data that is specific to the current scene being rendered.
		struct SceneData {
			math::Mat4 projectionMatrix;
			math::Mat4 viewMatrix;
			LightEnvironment lightEnvironment;
			Ref<StorageBuffer> lightSSBO;
		};

		inline static SceneData* s_SceneData = new Renderer::SceneData(); // Current scene data used for rendering.

		static void uploadLightEnvironment();
	};
} // namespace cbk::rendering
