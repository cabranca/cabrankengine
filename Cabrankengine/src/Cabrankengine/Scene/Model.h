#pragma once

#include "Mesh.h"

namespace cbk::scene {

	class Model {
	  public:
		Model(const std::string& path, const Ref<rendering::Material>& material);

		void draw(const math::Mat4& transform = math::identityMat());

		static Ref<Model> create(const std::string& path, const Ref<rendering::Material>& material);

	  private:
		std::vector<Mesh> m_Meshes;
		Ref<rendering::Material> m_Material;
	};

} // namespace cbk::scene
