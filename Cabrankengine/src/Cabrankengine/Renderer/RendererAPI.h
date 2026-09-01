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

	// RendererAPI is an abstract class that defines the interface for the low level rendering operations.
	class RendererAPI {
	  public:
		enum class API { None = 0, OpenGL = 1, Metal = 2, Vulkan = 3 }; // Enum representing the different rendering APIs supported.

		virtual ~RendererAPI() = default;

		// Initializes the renderer API. This method should be called before any rendering operations.
		virtual void init(const Window& window) = 0;

		virtual void shutdown() = 0;

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

		// Closes the pass scene geometry is drawn into and opens the one the UI is drawn
		// into. Backends that render straight to the backbuffer leave this empty; backends
		// that render the scene offscreen (so it can be displayed inside an ImGui window)
		// use it to end the offscreen pass and transition its result to a sampleable state.
		virtual void endScenePass() = 0;

		virtual void endFrame() = 0;

		// Sets the viewport dimensions for rendering.
		virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		// Returns the current rendering API.
		[[nodiscard]] static API getAPI() {
			return s_API;
		}

		// Backend handle for the texture the scene was rendered into, in a form ImGui can
		// consume as an ImTextureID. Returns 0 (ImTextureID_Invalid) on backends that render
		// straight to the backbuffer and therefore have no such texture.
		[[nodiscard]] virtual uint64_t getFinalFrame() const = 0;

	  private:
		static API s_API; // Static variable that holds the current rendering API being used.
	};
} // namespace cbk::rendering
