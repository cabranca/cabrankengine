#include <pch.h>
#include "OpenGLTexture2DMaterial.h"

namespace cbk::platform::opengl {

	OpenGLTexture2DMaterial::OpenGLTexture2DMaterial() {
		// The batch shader samples a uniform sampler array; bind each array entry
		// to its matching texture unit once at construction.
		m_Shader->bind();

		int32_t samplers[k_MaxTextureSlots];
		for (uint32_t i = 0; i < k_MaxTextureSlots; i++)
			samplers[i] = i;

		m_Shader->setIntArray("u_Textures", k_MaxTextureSlots, samplers);
	}

	void OpenGLTexture2DMaterial::setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) {
		CBK_CORE_ASSERT(slot < k_MaxTextureSlots, "OpenGLTexture2DMaterial: texture slot out of range");
		m_Slots[slot] = texture;
	}

	void OpenGLTexture2DMaterial::setViewProjection(const math::Mat4& viewProjection) {
		m_Shader->bind();
		m_Shader->setMat4("u_ViewProjection", viewProjection);
	}

	void OpenGLTexture2DMaterial::bind() const {
		m_Shader->bind();
		for (uint32_t i = 0; i < k_MaxTextureSlots; i++) {
			if (m_Slots[i])
				m_Slots[i]->bind(i);
		}
	}

} // namespace cbk::platform::opengl
