#include <pch.h>
#include "PBRModelArch.h"

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Registry.hpp>
#include <Cabrankengine/Renderer/Materials/PBRMaterial.h>

namespace cbk::scene::arch {

	using namespace ecs;

	PBRModelArch::PBRModelArch(std::string_view path) {
		auto& scene    = Application::get().getScene();
		m_Entity       = scene.createEntity("PBR Model");
		auto material  = rendering::PBRMaterial::create();
		auto model     = Model::create(std::string{ path }, material);
		scene.getRegistry()->addComponent(m_Entity, CTransform());
		scene.getRegistry()->addComponent(
		    m_Entity, CModel{ .Path = std::string{ path }, .Kind = common::MaterialKind::PBR, .Res = model });
	}

	CTransform& PBRModelArch::transform() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CTransform>(m_Entity).value();
	}

	CModel& PBRModelArch::model() {
		auto reg = Application::get().getRegistry();
		return *reg->getComponent<CModel>(m_Entity).value();
	}
} // namespace cbk::scene::arch
