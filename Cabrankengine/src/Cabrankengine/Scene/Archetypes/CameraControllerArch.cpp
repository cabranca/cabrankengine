#include <pch.h>
#include "CameraControllerArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>

namespace cbk::scene::arch {

	using namespace ecs;

	CameraControllerArch::CameraControllerArch(ProjectionType type) {
		auto& scene = Application::get().getScene();
		m_Entity = scene.createEntity("Camera Controller");
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(m_Entity, CCamera{ .Type = type });
		scene.getRegistry()->addComponent(m_Entity, CCameraController());
	}

	CTransform& CameraControllerArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CCamera& CameraControllerArch::camera() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CCamera>(m_Entity).value();
	}

    CCameraController& CameraControllerArch::controller() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CCameraController>(m_Entity).value();
	}
} // namespace cbk::scene::arch
