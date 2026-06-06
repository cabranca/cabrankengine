#pragma once

#include <pch.h>

#include <Common/Math/Mat4.h>
#include <Common/Math/Vector4.h>

namespace cbk::rendering {

	class GeometryDescriptor; // Forward declaration of VertexArray class.
	class Material;

	// RendererAPI is an abstract class that defines the interface for the low level rendering operations.
	class RendererAPI {
	  public:
		enum class API { None = 0, OpenGL = 1, Metal = 2, Vulkan = 3 }; // Enum representing the different rendering APIs supported.

		virtual ~RendererAPI() = default;

		// Initializes the renderer API. This method should be called before any rendering operations.
		virtual void init() = 0;

		virtual void shutdown() = 0;

		// Sets the color used to clear the screen.
		virtual void setClearColor(const math::Vector4& color) = 0;

		virtual void beginFrame() = 0;

		// Draws the vertex array vertices in order
		virtual void draw(const Ref<GeometryDescriptor>& vertexArray) = 0;

		// Draws the indexed vertices from the vertex array. The transform is the per-draw
		// model matrix — backends consume it differently (uniform on GL, push constant on Vulkan).
		virtual void drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& vertexArray, const math::Mat4& transform,
		                         uint32_t indexCount = 0) = 0;

		virtual void endFrame() = 0;

		// Sets the viewport dimensions for rendering.
		virtual void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		// Returns the current rendering API.
		[[nodiscard]] static API getAPI() {
			return s_API;
		}

	  private:
		static API s_API; // Static variable that holds the current rendering API being used.
	};
} // namespace cbk::rendering
