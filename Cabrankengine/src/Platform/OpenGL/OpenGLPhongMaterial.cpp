#include <pch.h>
#include "OpenGLPhongMaterial.h"

namespace cbk::platform::opengl {

	void OpenGLPhongMaterial::bind() const {
		m_Shader->bind();

		// "material.shininess" matches the struct field name in Phong.glsl.
		m_Shader->setFloat("material.shininess", m_Shininess);

		// Texture slots are by convention: 0 = diffuse, 1 = specular.
		if (m_DiffuseMap) {
			m_DiffuseMap->bind(0);
			m_Shader->setInt("material.diffuse", 0);
		}
		if (m_SpecularMap) {
			m_SpecularMap->bind(1);
			m_Shader->setInt("material.specular", 1);
		}
	}

} // namespace cbk::platform::opengl
