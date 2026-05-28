#pragma once

#include <pch.h>

namespace cbk::ecs {

	using Entity = uint32_t;

	constexpr Entity MAX_ENTITIES = 20000;
	constexpr Entity INVALID_ENTITY = std::numeric_limits<Entity>::max();
	constexpr size_t MAX_COMPONENTS = 64;

	using Signature = std::bitset<MAX_COMPONENTS>;
}

