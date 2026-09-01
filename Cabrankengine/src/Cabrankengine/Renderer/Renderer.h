#pragma once

#include <Common/Math/Mat4.h>
#include <Cabrankengine/Core/Window.h>

#include "RendererAPI.h"

namespace cbk::rendering {

	// Forward declarations
	class Material;
	class Shader;
	class GeometryDescriptor;

	// This is an abstracted API for rendering graphics in a game engine.
	// TODO: Re-think if this class makes sense already having RenderCommand and RendererAPI
	class Renderer {
	  public:
		// Initializes the renderer, setting up necessary resources and state.
		static void init(const Window& window);

		// Shuts down the renderer, releasing all resources and cleaning up state.
		static void shutdown();

		// Sets the necessary general data to render a scene, such as the camera, the lighting, etc.
		static void beginScene(const SceneData& sceneData);

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
	};
} // namespace cbk::rendering
