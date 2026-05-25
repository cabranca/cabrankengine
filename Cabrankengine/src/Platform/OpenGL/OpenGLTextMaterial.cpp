#include <pch.h>
#include "OpenGLTextMaterial.h"

namespace cbk::platform::opengl {

	OpenGLTextMaterial::OpenGLTextMaterial() {
		// The batch shader samples a uniform sampler array; bind each array entry
		// to its matching texture unit once at construction.
		m_Shader->bind();

		int32_t samplers[k_MaxTextureSlots];
		for (uint32_t i = 0; i < k_MaxTextureSlots; i++)
			samplers[i] = i;

		m_Shader->setIntArray("u_Textures", k_MaxTextureSlots, samplers);
	}

	void OpenGLTextMaterial::setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) {
		CBK_CORE_ASSERT(slot < k_MaxTextureSlots, "OpenGLTextMaterial: texture slot out of range");
		m_Slots[slot] = texture;
	}

	void OpenGLTextMaterial::setViewProjection(const math::Mat4& viewProjection) {
		m_Shader->bind();
		m_Shader->setMat4("u_ViewProjection", viewProjection);
	}

	void OpenGLTextMaterial::bind() const {
		m_Shader->bind();
		for (uint32_t i = 0; i < k_MaxTextureSlots; i++) {
			if (m_Slots[i])
				m_Slots[i]->bind(i);
		}
	}

} // namespace cbk::platform::opengl
