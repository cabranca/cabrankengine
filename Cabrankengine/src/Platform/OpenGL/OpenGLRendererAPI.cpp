#include <pch.h>
#include "OpenGLRendererAPI.h"

#include <glad/glad.h>

#include <Cabrankengine/Renderer/Materials/Material.h>

#include "OpenGLVertexArray.h"

namespace cbk::platform::opengl {

	using namespace rendering;

	void OpenGLRendererAPI::init() {
		CBK_PROFILE_FUNCTION();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	}

	void OpenGLRendererAPI::shutdown() {}

	void OpenGLRendererAPI::setClearColor(const math::Vector4& color) {
		glClearColor(color.x, color.y, color.z, color.w);
	}

	void OpenGLRendererAPI::clear() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::beginFrame() {}

	void OpenGLRendererAPI::draw(const Ref<GeometryDescriptor>& desc)
	{
		auto vao = static_cast<OpenGLVertexArray*>(desc.get());
		vao->bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void OpenGLRendererAPI::drawIndexed(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const math::Mat4& transform,
	                                    uint32_t indexCount) {
		material->bind();
		auto vao = static_cast<OpenGLVertexArray*>(desc.get());
		vao->bind();
		// Per-draw model matrix lives on the material's shader as a uniform. The shader-only
		// submit path handles its own setMat4, so we only touch the shader when a material is bound.
		if (material)
			material->getShader()->setMat4("u_Model", transform);
		uint32_t count = indexCount == 0 ? vao->getIndexCount() : indexCount;
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::endFrame() {}

	void OpenGLRendererAPI::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
		glViewport(x, y, width, height);
	}
}
