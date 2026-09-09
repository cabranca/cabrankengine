#pragma once

#include <pch.h>

#include <Common/Math/Mat4.h>
#include <Common/Math/Vector4.h>

#include <Cabrankengine/Core/Window.h>

namespace cbk::rendering {

	class GeometryDescriptor; // Forward declaration of VertexArray class.
	class Material;

	struct DirectionalLight {
		math::Vector3 Direction{ 0.f, -1.f, 0.f };
		// Defaults to zero radiance: a scene with no CDirectionalLight authored
		// gets no directional light, rather than a phantom white sun.
		math::Vector3 Radiance{ 0.f };
	};

	struct PointLight {
		math::Vector3 Position;
		math::Vector3 Radiance{ 1.f };

		// Standard Attenuation
		float Constant{ 1.f };
		float Linear{ 0.09f };
		float Quadratic{ 0.032f };
	};

	struct LightEnvironment {
		DirectionalLight DirLight;
		std::vector<PointLight> PointLights;
	};

	struct SceneData {
		math::Mat4 ViewProjectionMatrix;
		math::Vector3 CameraWorldPosition;
		struct LightEnvironment LightEnvironment;
	};

	// Startup-time renderer configuration. Everything here is fixed at init(): changing it later
	// would mean tearing down and rebuilding render targets while frames are still in flight.
	struct RendererSpec {
		// True renders the scene into an offscreen texture, which getFinalFrame() hands to ImGui so
		// it can be shown inside a viewport panel. False resolves the scene straight into the
		// swapchain image and composites the UI on top of it.
		bool RenderSceneToTexture = false;
	};

	// RendererAPI is an abstract class that defines the interface for the low level rendering operations.
	class RendererAPI {
	  public:
		enum class API { None = 0, OpenGL = 1, Metal = 2, Vulkan = 3 }; // Enum representing the different rendering APIs supported.

		virtual ~RendererAPI() = default;

		// Initializes the renderer API. This method should be called before any rendering operations.
		virtual void init(const Window& window, const RendererSpec& spec) = 0;

		virtual void shutdown() = 0;

		// Blocks until everything already submitted has finished on the GPU. Teardown needs
		// it: the layers and the scene release their GPU resources before the renderer is
		// shut down, and freeing a resource an in-flight frame still references is invalid.
		virtual void waitIdle() = 0;

		// Sets the color used to clear the screen.
		virtual void setClearColor(const math::Vector4& color) = 0;

		virtual void beginFrame() = 0;

		virtual void beginScene(const SceneData& sceneData) = 0;

		// Draws the vertex array vertices in order
		virtual void draw(const Ref<GeometryDescriptor>& vertexArray) = 0;

		// Draws the indexed vertices from the vertex array. The transform is the per-draw
		// model matrix — backends consume it differently (uniform on GL, push constant on Vulkan).
		virtual void drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& vertexArray, const math::Mat4& transform,
		                         uint32_t indexCount = 0) = 0;

		// Closes the pass scene geometry is drawn into and opens the one the UI is drawn into.
		// Backends that draw the UI into the same pass as the scene leave this empty. Where the
		// two are separate passes, this is also where an offscreen scene target is transitioned
		// to a sampleable state, so RendererSpec::RenderSceneToTexture is honoured here.
		virtual void endScenePass() = 0;

		virtual void endFrame() = 0;

		// Sets the viewport dimensions for rendering.
		virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		// Returns the current rendering API.
		[[nodiscard]] static API getAPI() {
			return s_API;
		}

		// Backend handle for the texture the scene was rendered into, in a form ImGui can
		// consume as an ImTextureID. Returns 0 (ImTextureID_Invalid) whenever the scene went
		// straight to the backbuffer — either because the backend has no offscreen path at all,
		// or because RendererSpec::RenderSceneToTexture was false — and there is therefore no
		// such texture. Callers use that as the signal to skip drawing a viewport panel.
		[[nodiscard]] virtual uint64_t getFinalFrame() const = 0;

	  private:
		static API s_API; // Static variable that holds the current rendering API being used.
	};
} // namespace cbk::rendering
