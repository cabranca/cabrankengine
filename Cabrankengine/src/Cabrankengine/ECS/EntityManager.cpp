#include <pch.h>
#include <Cabrankengine/ECS/EntityManager.h>

#include <Cabrankengine/Core/Logger.h>

namespace cbk::ecs {

	EntityManager::EntityManager() {
		for (Entity e = 0; e < MAX_ENTITIES; e++)
			m_AvailableEntities.push(e);
	}

	Entity EntityManager::createEntity() {
		if (m_LivingEntityCount >= MAX_ENTITIES) {
			CBK_CORE_WARN("Maximum entity count ({0}) reached. Entity not created.", MAX_ENTITIES);
			return INVALID_ENTITY;
		}
		Entity id = m_AvailableEntities.front();
		m_AvailableEntities.pop();
		m_LivingEntityCount++;
		return id;
	}

	void EntityManager::destroyEntity(Entity e) {
		CBK_CORE_ASSERT(e < MAX_ENTITIES, "Entity out of bound Identifiers!");
		m_Signatures[e].reset();
		m_AvailableEntities.push(e);
		m_LivingEntityCount--;
	}

	void EntityManager::setSignature(Entity e, Signature signature) {
		CBK_CORE_ASSERT(e < MAX_ENTITIES, "Entity out of bound Identifiers!");
		m_Signatures[e] = signature;
	}

	// I don't like that one could get the signature of a non existing entity (it would be 0, but still)
	Signature EntityManager::getSignature(Entity e) {
		CBK_CORE_ASSERT(e < MAX_ENTITIES, "Entity out of bound Identifiers!");
		return m_Signatures[e];
	}
}
