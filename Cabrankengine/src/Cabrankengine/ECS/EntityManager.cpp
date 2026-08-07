#include <pch.h>
#include <Cabrankengine/ECS/EntityManager.h>

#include <Cabrankengine/Core/Logger.h>

namespace cbk::ecs {

	EntityManager::EntityManager() {
		for (Entity e = 0; e < k_MaxEntities; e++)
			m_AvailableEntities.push(e);
	}

	Entity EntityManager::createEntity() {
		if (m_LivingEntityCount >= k_MaxEntities) {
			CBK_CORE_WARN("Maximum entity count ({0}) reached. Entity not created.", k_MaxEntities);
			return k_InvalidEntity;
		}
		Entity id = m_AvailableEntities.front();
		m_AvailableEntities.pop();
		m_LivingEntityCount++;
		CBK_CORE_TRACE("Entity created: {0}", id);
		return id;
	}

	void EntityManager::destroyEntity(Entity e) {
		if (e >= k_MaxEntities) {
			CBK_CORE_ERROR("destroyEntity: entity {0} out of bounds (max {1})", e, k_MaxEntities);
			return;
		}
		m_Signatures[e].reset();
		m_AvailableEntities.push(e);
		m_LivingEntityCount--;
		CBK_CORE_TRACE("Entity destroyed: {0}", e);
	}

	void EntityManager::setSignature(Entity e, Signature signature) {
		if (e >= k_MaxEntities) {
			CBK_CORE_ERROR("setSignature: entity {0} out of bounds (max {1})", e, k_MaxEntities);
			return;
		}
		m_Signatures[e] = signature;
		CBK_CORE_TRACE("Signature set on entity {0}: {1}", e, signature.to_string());
	}

	// I don't like that one could get the signature of a non existing entity (it would be 0, but still)
	Signature EntityManager::getSignature(Entity e) {
		if (e >= k_MaxEntities) {
			CBK_CORE_ERROR("getSignature: entity {0} out of bounds (max {1})", e, k_MaxEntities);
			return Signature{};
		}
		return m_Signatures[e];
	}
} // namespace cbk::ecs
