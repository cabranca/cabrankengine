#include <pch.h>
#include "TextArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>

namespace cbk::scene::arch {

	using namespace ecs;

	TextArch::TextArch() {
		auto& scene = Application::get().getScene();
		m_Entity = scene.createEntity();
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(m_Entity, CText());
	}

	CTransform& TextArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CText& TextArch::text() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CText>(m_Entity).value();
	}
} // namespace cbk::scene::arch
