#pragma once

#include <Common/Math/MatrixFactory.h>
#include <Cabrankengine/Renderer/Materials/Material.h>
#include <Cabrankengine/Renderer/Shader.h>
#include <Cabrankengine/Renderer/Texture.h>
#include <Cabrankengine/Renderer/GeometryDescriptor.h>

namespace cbk::scene {

	enum class TextureType { None = 0, Diffuse, Specular, Normal, Height, Ambient };

	class Mesh {
	  public:
		Mesh(std::vector<rendering::Vertex> vertices, std::vector<uint32_t> indices, Ref<rendering::Material> material);
		void draw(const math::Mat4& transform);

	  private:
		Ref<rendering::GeometryDescriptor> m_Descriptor;
		Ref<rendering::Material> m_Material;
	};
} // namespace cbk::scene