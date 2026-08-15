#pragma once

#include <Cabrankengine/Renderer/Shader.h>
#include <Cabrankengine/Renderer/Texture.h>
#include <Cabrankengine/Scene/DefaultLibrary.h>

namespace cbk::rendering {

	class Material {
	  public:
		Material(const Ref<Shader>& shader) : m_Shader(shader) {}
		virtual ~Material() = default;

		// Hooks consumed by model loading. Concrete materials override to wire a
		// (TextureType, key) → setter mapping. Default is no-op.
		virtual void applyTexture(common::TextureType type, const Ref<Texture2D>& texture) {}
		virtual void applyProperty(uint32_t key, float value) {}

		// Returns a fresh, blank material of the same concrete type. Used by model
		// loading to give each material slot its own instance. Returns nullptr if
		// the concrete type does not support it.
		[[nodiscard]] virtual Ref<Material> instantiate() const {
			return nullptr;
		}

		[[nodiscard]] Ref<Shader> getShader() const {
			return m_Shader;
		}

	  protected:
		Ref<Shader> m_Shader;
	};

} // namespace cbk::rendering