#pragma once

#include "Common.h"

namespace cbk::ecs {

	/// <summary>
	/// This class is supposed to handle Entities creation, destruction and signature setting.
	/// It also recycles the ids of the dead entities.
	/// </summary>
	class EntityManager {
	  public:
		// Initializes the available entities
		EntityManager();

		// Creates an entity and returns its ID
		Entity createEntity();

		// Destroys the given entity and recycles its ID
		void destroyEntity(Entity e);

		// Sets the whole components signature to the given entity
		void setSignature(Entity e, Signature signature);

		// Returns the components signature for the given entity
		Signature getSignature(Entity e);

	  private:
		std::queue<Entity> m_AvailableEntities{};            // Recycled queue of available IDs
		std::array<Signature, k_MaxEntities> m_Signatures{}; // Array of signatures corresponding to the ids of their indexes.
		uint32_t m_LivingEntityCount = 0;                    // Counter for the current living entities
	};
} // namespace cbk::ecs
