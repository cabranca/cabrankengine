#pragma once

#include "EntityManager.h"
#include "ComponentManager.hpp"
#include "SystemManager.hpp"

namespace cbk::ecs {

	/// <summary>
	/// Manages entities and their components in an entity-component-system (ECS) architecture.
	/// </summary>
	class Registry {
	  public:
		Registry() {
			m_ComponentManager = std::make_unique<ComponentManager>();
			m_EntityManager = std::make_unique<EntityManager>();
			m_SystemManager = std::make_unique<SystemManager>();
		}

		//////////ENTITY//////////

		// Creates a new entity and returns its ID
		Entity createEntity() {
			return m_EntityManager->createEntity();
		}

		// Destroys the given entity and removes all its components
		void destroyEntity(Entity e) {
			m_EntityManager->destroyEntity(e);
			m_ComponentManager->entityDestroyed(e);
			m_SystemManager->entityDestroyed(e);
		}

		// Returns the components signature for the given entity
		Signature getSignature(Entity e) {
			return m_EntityManager->getSignature(e);
		}

		/////////////////////////////

		//////////COMPONENT//////////

		// Registers a new component type T
		template <typename T>
		void registerComponent() {
			m_ComponentManager->registerComponent<T>();
		}

		// Adds a component of type T to the given entity
		template <typename T>
		void addComponent(Entity e, T component) {
			m_ComponentManager->addComponent<T>(e, component);

			// Instead of setting the bit in the signature, copies, sets and copies again
			auto signature = m_EntityManager->getSignature(e);
			signature.set(m_ComponentManager->getComponentType<T>(), true);
			m_EntityManager->setSignature(e, signature);

			m_SystemManager->entitySignatureChanged(e, signature);
		}

		// Removes the component of type T from the given entity
		template <typename T>
		void removeComponent(Entity e) {
			m_ComponentManager->removeComponent<T>(e);

			auto signature = m_EntityManager->getSignature(e);
			signature.set(m_ComponentManager->getComponentType<T>(), false);
			m_EntityManager->setSignature(e, signature);

			m_SystemManager->entitySignatureChanged(e, signature);
		}

		// Returns a reference to the component of type T associated with the given entity
		template <typename T>
		std::optional<T*> getComponent(Entity e) {
			return m_ComponentManager->getComponent<T>(e);
		}

		template <typename T>
		std::optional<const T*> getComponent(Entity e) const {
			const ComponentManager* cm = m_ComponentManager.get();
			return cm->getComponent<T>(e);
		}

		// Returns the unique ID assigned to the component type T
		template <typename T>
		uint8_t getComponentType() {
			return m_ComponentManager->getComponentType<T>();
		}

		///////////////////////////

		//////////SYSTEM//////////

		// Registers a new system of type T
		template <typename T>
		std::shared_ptr<T> registerSystem() {
			return m_SystemManager->registerSystem<T>();
		}

		template <typename T>
		std::shared_ptr<T> getSystem() {
			return m_SystemManager->getSystem<T>();
		}

		// Sets the required component signature for a given system type T
		template <typename T>
		void setSystemSignature(Signature signature) {
			m_SystemManager->setSignature<T>(signature);
		}

		// Re-evaluates all existing entities against registered system signatures.
		// Call this after registering systems on a registry that already has entities (e.g. after scene load).
		void rebuildSystemMembership() {
			for (Entity e = 0; e < k_MaxEntities; e++) {
				Signature sig = m_EntityManager->getSignature(e);
				if (sig.any())
					m_SystemManager->entitySignatureChanged(e, sig);
			}
		}

	  private:
		std::unique_ptr<ComponentManager> m_ComponentManager; // Manages component storage and access
		std::unique_ptr<EntityManager> m_EntityManager;       // Manages entity creation and destruction
		std::unique_ptr<SystemManager> m_SystemManager;       // Manages systems creation and destruction
	};
} // namespace cbk::ecs
