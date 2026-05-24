#include <pch.h>

#include <Cabrankengine/Renderer/Renderer.h>

#include "Mesh.h"

namespace cbk::scene {

	using namespace math;
	using namespace rendering;

	Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Ref<Material> material) : m_Material(material) {
		m_Descriptor = GeometryDescriptor::create(vertices.data(), vertices.size() * sizeof(rendering::Vertex), indices.data(),
		                                          indices.size() * sizeof(uint32_t),
		                                          { { ShaderDataType::Float3, "pos" },
		                                            { ShaderDataType::Float3, "normal" },
		                                            { ShaderDataType::Float2, "texCoords" },
		                                            { ShaderDataType::Float3, "tangent" } });
	}

	void Mesh::draw(const math::Mat4& transform) {
		m_Material->getShader()->setMat4("u_Model", transform);
		Renderer::submit(m_Material, m_Descriptor, transform);
	}
} // namespace cbk::scene
