#pragma once

#include <Cabrankengine/ECS/Common.h>
#include <Cabrankengine/ECS/Components.h>

namespace cbk::scene::arch {

	class PhongModelArch {
	  public:
		PhongModelArch(std::string_view path);

		ecs::CTransform& transform();
		ecs::CPhongModel& model();

	  private:
		ecs::Entity m_Entity;
	};
} // namespace cbk::scene::arch
