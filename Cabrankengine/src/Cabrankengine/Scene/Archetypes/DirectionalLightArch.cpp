#include <pch.h>
#include "DirectionalLightArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>

namespace cbk::scene::arch {

	using namespace ecs;

	DirectionalLightArch::DirectionalLightArch() {
		auto& scene = Application::get().getScene();
		m_Entity = scene.createEntity("Directional Light");
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(m_Entity, CDirectionalLight());
	}

	CDirectionalLight& DirectionalLightArch::light() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CDirectionalLight>(m_Entity).value();
	}
} // namespace cbk::scene::arch
