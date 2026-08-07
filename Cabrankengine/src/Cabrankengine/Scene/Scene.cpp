#include <pch.h>
#include "Scene.h"
#include <Cabrankengine/ECS/Components.h>

namespace cbk::scene {

	using namespace ecs;
	using namespace math;

	static void registerBuiltInComponents(Registry& reg) {
		reg.registerComponent<CTransform>();
		reg.registerComponent<CCamera>();
		reg.registerComponent<CCameraController>();
		reg.registerComponent<CDirectionalLight>();
		reg.registerComponent<CPointLight>();
		reg.registerComponent<CSprite>();
		reg.registerComponent<CModel>();
		reg.registerComponent<CText>();
	}

	Scene::Scene() {
		registerBuiltInComponents(m_Registry);
	}

	Scene::Scene(SceneMetadata metadata) : m_Metadata(std::move(metadata)) {
		registerBuiltInComponents(m_Registry);
	}

	Entity Scene::createEntity(const std::string& name) {
		auto entity = m_Registry.createEntity();
		m_Entities.push_back(entity);
		m_EntityToName[entity] = name;
		return entity;
	}

	void Scene::destroyEntity(Entity e) {
		m_Registry.destroyEntity(e);
		std::erase(m_Entities, e);
		m_EntityToName.erase(e);
	}

	Entity Scene::findEntityByName(std::string_view name) const {
		for (auto& [entity, n]: m_EntityToName)
			if (n == name)
				return entity;

		CBK_CORE_WARN("findEntityByName: there is no entity with the given name ({})", name);
		return k_InvalidEntity;
	}

	std::span<const Entity> Scene::getAllEntities() const {
		return m_Entities;
	}

	std::string_view Scene::getEntityName(Entity e) const {
		auto it = m_EntityToName.find(e);
		CBK_CORE_ASSERT(it != m_EntityToName.end(), "getEntityName: entity does not exist!");
		return it->second;
	}

	void Scene::setEntityName(Entity e, const std::string& name) {
		auto it = m_EntityToName.find(e);
		CBK_CORE_ASSERT(it != m_EntityToName.end(), "setEntityName: entity does not exist!");
		it->second = name;
	}

	Registry* Scene::getRegistry() {
		return &m_Registry;
	}

	const Registry* Scene::getRegistry() const {
		return &m_Registry;
	}

	const SceneMetadata& Scene::getMetadata() const {
		return m_Metadata;
	}
} // namespace cbk::scene