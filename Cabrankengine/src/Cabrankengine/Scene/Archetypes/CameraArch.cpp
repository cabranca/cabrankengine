#include <pch.h>
#include "CameraArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>

namespace cbk::scene::arch {

	using namespace ecs;

	CameraArch::CameraArch(ProjectionType type) {
		auto& scene = Application::get().getScene();
		m_Entity = scene.createEntity("Camera");
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(m_Entity, CCamera{ .Type = type });
	}

	CTransform& CameraArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CCamera& CameraArch::camera() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CCamera>(m_Entity).value();
	}
} // namespace cbk::scene::arch
