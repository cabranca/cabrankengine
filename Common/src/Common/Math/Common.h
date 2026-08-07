#pragma once

#include <numbers>

namespace cbk::math {

	inline constexpr float k_Pi = std::numbers::pi_v<float>;
	inline constexpr float k_HalfPi = std::numbers::pi_v<float> / 2.0f;
	inline constexpr float k_TwoPi = std::numbers::pi_v<float> * 2.0f;
	inline constexpr float k_Epsilon = 1e-5f;

	inline float radians(float degrees) {
		return degrees * k_Pi / 180.f;
	}
} // namespace cbk::math
