#pragma once

#include <Cabrankengine/ECS/Common.h>
#include <Cabrankengine/ECS/Components.h>

namespace cbk::scene::arch {

	class SpriteArch {
	  public:
		SpriteArch(std::string_view path);

		ecs::CTransform& transform();
		ecs::CSprite& sprite();

	  private:
		ecs::Entity m_Entity;
	};
} // namespace cbk::scene::arch
