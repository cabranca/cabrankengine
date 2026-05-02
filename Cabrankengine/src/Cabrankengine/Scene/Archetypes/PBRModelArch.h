#pragma once

#include <Cabrankengine/ECS/Common.h>
#include <Cabrankengine/ECS/Components.h>

namespace cbk::scene::arch {

	class PBRModelArch {
	  public:
		PBRModelArch(std::string_view path);

		ecs::CTransform& transform();
		ecs::CPBRModel& model();

	  private:
		ecs::Entity m_Entity;
	};
} // namespace cbk::scene::arch
