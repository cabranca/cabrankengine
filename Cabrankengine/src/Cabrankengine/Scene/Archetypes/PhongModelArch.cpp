#include <pch.h>
#include "PhongModelArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>
#include <Cabrankengine/Renderer/Materials/PhongMaterial.h>

namespace cbk::scene::arch {

	using namespace ecs;

	PhongModelArch::PhongModelArch(std::string_view path) {
		auto& scene    = Application::get().getScene();
		m_Entity       = scene.createEntity("Phong Model");
		auto material  = rendering::PhongMaterial::create();
		auto model     = Model::create(std::string{ path }, material);
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(
		    m_Entity, CModel{ .Path = std::string{ path }, .Kind = common::MaterialKind::Phong, .Res = model });
	}

	CTransform& PhongModelArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CModel& PhongModelArch::model() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CModel>(m_Entity).value();
	}
} // namespace cbk::scene::arch
