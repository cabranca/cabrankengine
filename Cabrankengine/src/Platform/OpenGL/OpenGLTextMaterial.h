#pragma once

#include <Cabrankengine/Renderer/Materials/TextMaterial.h>

namespace cbk::platform::opengl {

	// OpenGL backing for the TextRenderer glyph batch pipeline. Mirrors
	// OpenGLTexture2DMaterial; texture slots map directly to texture units.
	class OpenGLTextMaterial : public rendering::TextMaterial {
	  public:
		static constexpr uint32_t k_MaxTextureSlots = 32;

		OpenGLTextMaterial();

		void bind() const override;

		void setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) override;
		void setViewProjection(const math::Mat4& viewProjection) override;

	  private:
		std::array<Ref<rendering::Texture2D>, k_MaxTextureSlots> m_Slots{};
	};

} // namespace cbk::platform::opengl
