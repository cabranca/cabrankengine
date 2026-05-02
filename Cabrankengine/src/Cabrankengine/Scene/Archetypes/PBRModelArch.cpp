#include <pch.h>
#include "PBRModelArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>

namespace cbk::scene::arch {

	using namespace ecs;

	PBRModelArch::PBRModelArch() {
		auto& scene = Application::get().getScene();
		m_Entity = scene.createEntity();
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(m_Entity, CPBRModel());
	}

	CTransform& PBRModelArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CPBRModel& PBRModelArch::model() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CPBRModel>(m_Entity).value();
	}
} // namespace cbk::scene::arch
