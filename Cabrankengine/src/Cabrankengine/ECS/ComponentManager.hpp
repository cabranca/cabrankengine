#pragma once

#include <Cabrankengine/Core/Logger.h>
#include "Common.h"

namespace cbk::ecs {

	/// <summary>
	/// This interface is needed because the ComponentManager has a collection of templated component arrays.
	/// In order to keep it generic, the collection must be of an abstract type.
	/// </summary>
	class IComponentArray {
	  public:
		virtual ~IComponentArray() = default;

		// This must be called after a call to EntityManager::destroyEntity()
		virtual void entityDestroyed(Entity e) = 0;
	};

	/// <summary>
	/// A Component Array represents a collection of the entities that hold the component of type T
	/// </summary>
	template <typename T>
	class ComponentArray : public IComponentArray {
	  public:
		// Associates the given component and entity
		void insert(Entity e, T component) {
			if (m_EntityToIndex.contains(e)) {
				CBK_CORE_ERROR("ComponentArray<{0}>::insert: component already added to entity {1}", typeid(T).name(), e);
				return;
			}
			size_t index = m_Size;
			m_EntityToIndex[e] = index;
			m_IndexToEntity[index] = e;
			m_Components[index] = component;
			m_Size++;
			CBK_CORE_TRACE("Component {0} added to entity {1}", typeid(T).name(), e);
		}

		// Removes the component related to the given entity and any record of it
		void remove(Entity e) {
			if (!m_EntityToIndex.contains(e)) {
				CBK_CORE_ERROR("ComponentArray<{0}>::remove: component does not exist on entity {1}", typeid(T).name(), e);
				return;
			}
			size_t index = m_EntityToIndex[e];
			size_t lastIndex = m_Size - 1;
			m_Components[index] = m_Components[lastIndex];
			Entity lastEntity = m_IndexToEntity[lastIndex];
			m_EntityToIndex[lastEntity] = index;
			m_IndexToEntity[index] = lastEntity;
			m_EntityToIndex.erase(e);
			m_IndexToEntity.erase(lastIndex);
			m_Size--;
			CBK_CORE_TRACE("Component {0} removed from entity {1}", typeid(T).name(), e);
		}

		// Returns the component for the given entity
		std::optional<T*> get(Entity e) {
			if (!m_EntityToIndex.contains(e))
				return std::nullopt;
			return &m_Components[m_EntityToIndex[e]];
		}

		std::optional<const T*> get(Entity e) const {
			if (!m_EntityToIndex.contains(e))
				return std::nullopt;
			return &m_Components[m_EntityToIndex.at(e)];
		}

		// This must be called after a call to EntityManager::destroyEntity()
		void entityDestroyed(Entity e) override {
			if (m_EntityToIndex.contains(e))
				remove(e);
		}

	  private:
		std::array<T, MAX_ENTITIES> m_Components{};         // The actual component array
		std::unordered_map<Entity, size_t> m_EntityToIndex; // Map from entity ID to the index of the component in the array
		std::unordered_map<size_t, Entity> m_IndexToEntity; // Map from the index of the component in the array to the entity ID
		size_t m_Size = 0;                                  // The current amount of valid components in the array
	};

	/// <summary>
	/// Manages registration, storage, and access of components for entities in an entity-component system.
	/// </summary>
	class ComponentManager {
	  public:
		// Registers a component type T and assigns it a unique ID (idempotent)
		template <typename T>
		void registerComponent() {
			const char* typeName = typeid(T).name();
			if (m_ComponentTypes.contains(typeName)) {
				CBK_CORE_ERROR("Component {} added twice!", typeName);
				return;
			}

			m_ComponentTypes[typeName] = m_NextComponentType++;
			m_ComponentArrays[typeName] = std::make_shared<ComponentArray<T>>();
			CBK_CORE_TRACE("Component registered: {0}", typeName);
		}

		// Returns the unique ID assigned to the component type T
		template <typename T>
		uint8_t getComponentType() {
			const char* typeName = typeid(T).name();
			if (!m_ComponentTypes.contains(typeName)) {
				CBK_CORE_ERROR("getComponentType: component {0} not registered", typeName);
				return 0;
			}
			return m_ComponentTypes[typeName];
		}

		// Adds a component of type T to the given entity
		template <typename T>
		void addComponent(Entity e, T component) {
			auto array = getComponentArray<T>();
			if (!array)
				return;
			array->insert(e, component);
		}

		// Removes the component of type T from the given entity
		template <typename T>
		void removeComponent(Entity e) {
			auto array = getComponentArray<T>();
			if (!array)
				return;
			array->remove(e);
		}

		// Returns the component of type T associated with the given entity, if present
		template <typename T>
		std::optional<T*> getComponent(Entity e) {
			auto array = getComponentArray<T>();
			if (!array)
				return std::nullopt;
			return array->get(e);
		}

		template <typename T>
		std::optional<const T*> getComponent(Entity e) const {
			auto array = getComponentArray<T>();
			if (!array)
				return std::nullopt;
			return array->get(e);
		}

		// This must be called after a call to EntityManager::destroyEntity()
		void entityDestroyed(Entity e) {
			for (auto const& pair: m_ComponentArrays)
				pair.second->entityDestroyed(e);
		}

	  private:
		std::unordered_map<const char*, uint8_t> m_ComponentTypes; // Map from component type string to its ID
		std::unordered_map<const char*, std::shared_ptr<IComponentArray>>
		    m_ComponentArrays;           // Map from component type string to its component array
		uint8_t m_NextComponentType = 0; // The ID to be assigned to the next registered component type

		// Returns the component array for the component type T
		template <typename T>
		std::shared_ptr<ComponentArray<T>> getComponentArray() {
			const char* typeName = typeid(T).name();
			if (!m_ComponentTypes.contains(typeName)) {
				CBK_CORE_ERROR("getComponentArray: component {0} not registered", typeName);
				return nullptr;
			}
			return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays[typeName]);
		}

		template <typename T>
		std::shared_ptr<const ComponentArray<T>> getComponentArray() const {
			const char* typeName = typeid(T).name();
			if (!m_ComponentTypes.contains(typeName)) {
				CBK_CORE_ERROR("getComponentArray: component {0} not registered", typeName);
				return nullptr;
			}
			return std::static_pointer_cast<ComponentArray<T>>(m_ComponentArrays.at(typeName));
		}
	};
} // namespace cbk::ecs