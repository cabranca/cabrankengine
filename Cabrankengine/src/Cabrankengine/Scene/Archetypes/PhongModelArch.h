#pragma once

#include <Cabrankengine/ECS/Common.h>
#include <Cabrankengine/ECS/Components.h>

namespace cbk::scene::arch {

	class PhongModelArch {
	  public:
		PhongModelArch(std::string_view path);

		ecs::CTransform& transform();
		ecs::CModel&     model();

	  private:
		ecs::Entity m_Entity;
	};
} // namespace cbk::scene::arch
