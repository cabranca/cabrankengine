#include <pch.h>
#include "PhongModelArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>

namespace cbk::scene::arch {

	using namespace ecs;

	PhongModelArch::PhongModelArch(std::string_view path) {
		auto& scene = Application::get().getScene();
		m_Entity = scene.createEntity();
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(m_Entity,
		                                  CPhongModel{ .Path = path.data(), .Res = Model<rendering::PhongMaterial>::create(path.data()) });
	}

	CTransform& PhongModelArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CPhongModel& PhongModelArch::model() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CPhongModel>(m_Entity).value();
	}
} // namespace cbk::scene::arch
