#pragma once

#include <Cabrankengine/ECS/Common.h>
#include <Cabrankengine/ECS/Components.h>

namespace cbk::scene::arch {

	class CameraControllerArch {
	  public:
		CameraControllerArch(ecs::ProjectionType type);

		ecs::CTransform& transform();
		ecs::CCamera& camera();
		ecs::CCameraController& controller();

	  private:
		ecs::Entity m_Entity;
	};
} // namespace cbk::scene::arch
