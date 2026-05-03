#pragma once

#include <Cabrankengine/ECS/Registry.hpp>
#include <Cabrankengine/Math/Vector4.h>

namespace cbk::scene {

	struct SceneMetadata {
		std::string Name = "DefaultScene";
		math::Vector4 BackgroundColor{ 0.2f, 0.2f, 0.2f, 1.f };
		math::Vector4 AmbientColor{ 1.f };
	};

	class Scene {
	  public:
		Scene();
		explicit Scene(SceneMetadata metadata);

		[[nodiscard]] ecs::Entity createEntity(const std::string& name);
		void destroyEntity(ecs::Entity e);

		[[nodiscard]] ecs::Entity findEntityByName(std::string_view name) const;
		[[nodiscard]] std::span<const ecs::Entity> getAllEntities() const;

		[[nodiscard]] std::string_view getEntityName(ecs::Entity e) const;
		void setEntityName(ecs::Entity e, const std::string& name);

		[[nodiscard]] ecs::Registry* getRegistry();
		[[nodiscard]] const ecs::Registry* getRegistry() const;
		[[nodiscard]] const SceneMetadata& getMetadata() const; // TODO: non-const getter or name setter

	  private:
		ecs::Registry m_Registry;
		SceneMetadata m_Metadata;
		std::vector<ecs::Entity> m_Entities;
		std::unordered_map<ecs::Entity, std::string> m_EntityToName;
	};
} // namespace cbk::scene