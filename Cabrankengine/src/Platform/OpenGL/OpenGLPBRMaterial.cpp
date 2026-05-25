#include <pch.h>
#include "OpenGLPBRMaterial.h"

namespace cbk::platform::opengl {

	void OpenGLPBRMaterial::bind() const {
		m_Shader->bind();

		m_Shader->setFloat3("u_AlbedoColor", m_AlbedoColor);
		m_Shader->setFloat("u_Metalness", m_Metalness);
		m_Shader->setFloat("u_Roughness", m_Roughness);

		// PBR slot convention: 0 = Albedo, 1 = Normal, 2 = MetalRoughness, 3 = AO.
		m_AlbedoMap->bind(0);
		m_Shader->setInt("u_AlbedoMap", 0);

		m_NormalMap->bind(1);
		m_Shader->setInt("u_NormalMap", 1);

		m_MetalRoughMap->bind(2);
		m_Shader->setInt("u_MetalRoughMap", 2);

		m_AOMap->bind(3);
		m_Shader->setInt("u_AOMap", 3);
	}

} // namespace cbk::platform::opengl
