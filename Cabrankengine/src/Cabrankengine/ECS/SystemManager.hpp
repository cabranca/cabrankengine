#pragma once

#include <Cabrankengine/Core/Logger.h>
#include "Common.h"

namespace cbk::ecs {

	class Registry;

	/// <summary>
	/// Represents a system containing a set of entities.
	/// </summary>
	class ISystem {
	  public:
		virtual ~ISystem() = default;

		// Update method to be implemented by derived systems
		virtual void update(Registry& registry, float dt) = 0;

		// Returns the entities managed by this system
		const std::unordered_set<Entity>& getEntities() const {
			return m_Entities;
		}

		// Adds the given entity to the system
		void addEntity(Entity entity) {
			m_Entities.emplace(entity);
		}

		// Removes the given entity from the system
		void removeEntity(Entity entity) {
			m_Entities.erase(entity);
		}

	  protected:
		std::unordered_set<Entity> m_Entities; // Set of entities managed by the system
	};

	/// <summary>
	/// SystemManager is responsible for managing and updating all systems in the ECS architecture.
	/// It holds a collection of systems and provides methods to add, remove, and update them.
	/// </summary>
	class SystemManager {
	  public:
		// Registers a system of type T and returns a shared pointer to it
		template <typename T>
		std::shared_ptr<T> RegisterSystem() {
			const char* typeName = typeid(T).name();

			if (m_Systems.contains(typeName)) {
				CBK_CORE_ERROR("RegisterSystem: system {0} already registered", typeName);
				return std::static_pointer_cast<T>(m_Systems.at(typeName));
			}

			auto system = std::make_shared<T>();
			m_Systems.emplace(typeName, system);
			CBK_CORE_TRACE("System registered: {0}", typeName);
			return system;
		}

		template <typename T>
		std::shared_ptr<T> getSystem() {
			const char* typeName = typeid(T).name();

			if (!m_Systems.contains(typeName)) {
				CBK_CORE_ERROR("getSystem: system {0} not registered", typeName);
				return nullptr;
			}

			return std::static_pointer_cast<T>(m_Systems.at(typeName));
		}

		// Sets the signature for the system of type T
		template <typename T>
		void SetSignature(Signature signature) {
			const char* typeName = typeid(T).name();

			if (!m_Systems.contains(typeName)) {
				CBK_CORE_ERROR("SetSignature: system {0} not registered", typeName);
				return;
			}

			m_Signatures.emplace(typeName, signature);
			CBK_CORE_TRACE("System signature set: {0}", typeName);
		}

		// This must be called after a call to EntityManager::destroyEntity()
		void EntityDestroyed(Entity entity) {
			for (auto const& [_, system]: m_Systems) {
				system->removeEntity(entity);
			}
		}

		// This must be called after a call to EntityManager::setSignature()
		void EntitySignatureChanged(Entity entity, Signature entitySignature) {
			for (auto const& [type, system]: m_Systems) {
				auto const& systemSignature = m_Signatures[type];

				if ((entitySignature & systemSignature) == systemSignature)
					system->addEntity(entity);
				else
					system->removeEntity(entity);
			}
		}

	  private:
		std::unordered_map<const char*, Signature> m_Signatures;
		std::unordered_map<const char*, std::shared_ptr<ISystem>> m_Systems;
	};
} // namespace cbk::ecs