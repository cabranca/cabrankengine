#pragma once

#include <array>

#include <Cabrankengine/Renderer/Materials/Texture2DMaterial.h>

namespace cbk::platform::opengl {

	// OpenGL backing for the Renderer2D batch quad pipeline. Texture slots map
	// directly to texture units; the batch shader samples a uniform sampler array.
	class OpenGLTexture2DMaterial : public rendering::Texture2DMaterial {
	  public:
		static constexpr uint32_t k_MaxTextureSlots = 32;

		OpenGLTexture2DMaterial();

		void bind() const override;

		void setTextureSlot(uint32_t slot, const Ref<rendering::Texture2D>& texture) override;
		void setViewProjection(const math::Mat4& viewProjection) override;

	  private:
		std::array<Ref<rendering::Texture2D>, k_MaxTextureSlots> m_Slots{};
	};

} // namespace cbk::platform::opengl
