#pragma once

#include <Cabrankengine/Renderer/Materials/PhongMaterial.h>

namespace cbk::platform::opengl {

	class OpenGLPhongMaterial : public rendering::PhongMaterial {
	  public:
		OpenGLPhongMaterial() = default;

		void bind() const override;
	};

} // namespace cbk::platform::opengl
