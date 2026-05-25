#pragma once

#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>

namespace cbk::platform::opengl {

	class OpenGLPBRMaterial : public rendering::PBRMaterial {
	  public:
		OpenGLPBRMaterial() = default;

		void bind() const override;
	};

} // namespace cbk::platform::opengl
