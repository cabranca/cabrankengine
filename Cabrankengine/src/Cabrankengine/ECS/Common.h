#pragma once

#include <pch.h>

namespace cbk::ecs {

	using Entity = uint32_t;

	constexpr Entity k_MaxEntities = 20000;
	constexpr Entity k_InvalidEntity = std::numeric_limits<Entity>::max();
	constexpr size_t k_MaxComponents = 64;

	using Signature = std::bitset<k_MaxComponents>;
} // namespace cbk::ecs
