#include <pch.h>
#include "Scene.h"

namespace cbk::scene {

	using namespace ecs;
	using namespace math;

	Scene::Scene(SceneMetadata metadata) : m_Metadata(std::move(metadata)) {}

	Entity Scene::createEntity() {
		return m_Registry.createEntity();
	}

	void Scene::destroyEntity(Entity e) {
		m_Registry.destroyEntity(e);
	}

	Entity Scene::findEntityByName(std::string_view name) const {
		for (auto& [entity, n]: m_EntityToName)
			if (n == name)
				return entity;

		CBK_CORE_WARN("findEntityByName: there is no entity with the given name ({})", name);
		return INVALID_ENTITY;
	}

	std::span<Entity> Scene::getAllEntities() const {
		return {};
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

	const SceneMetadata& Scene::getMetadata() const {
		return m_Metadata;
	}
} // namespace cbk::scene