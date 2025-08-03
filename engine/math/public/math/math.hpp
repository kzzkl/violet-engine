#pragma once

#include "math/types.hpp"
#include <cmath>
#include <utility>

namespace violet::math
{
static constexpr float PI = 3.141592654f;
static constexpr float TWO_PI = 2.0f * PI;
static constexpr float INV_PI = 1.0f / PI;
static constexpr float INV_TWO_PI = 1.0f / TWO_PI;
static constexpr float HALF_PI = PI / 2.0f;
static constexpr float QUARTER_PI = PI / 4.0f;
static constexpr float DEG_TO_RAD = PI / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / PI;

[[nodiscard]] static inline float to_radians(float degrees)
{
    return degrees * math::DEG_TO_RAD;
}

[[nodiscard]] static inline float to_degrees(float radians)
{
    return radians * math::RAD_TO_DEG;
}

[[nodiscard]] static inline std::pair<float, float> sin_cos(float radians)
{
    float temp = radians * math::INV_TWO_PI;
    if (temp > 0.0f)
    {
        temp = static_cast<float>(std::lround(temp));
    }
    else
    {
        temp = static_cast<float>(static_cast<int>(temp - 0.5f));
    }

    float x = radians - (math::TWO_PI * temp);

    float sign = 1.0f;
    if (x > math::HALF_PI)
    {
        x = math::PI - x;
        sign = -1.0f;
    }
    else if (x < -math::HALF_PI)
    {
        x = -math::PI - x;
        sign = -1.0f;
    }

    float x2 = x * x;

    float sin =
        (((((-2.3889859e-08f * x2 + 2.7525562e-06f) * x2 - 0.00019840874f) * x2 + 0.0083333310f) *
              x2 -
          0.16666667f) *
             x2 +
         1.0f) *
        x;

    float cos =
        (((((-2.6051615e-07f * x2 + 2.4760495e-05f) * x2 - 0.0013888378f) * x2 + 0.041666638f) *
              x2 -
          0.5f) *
         x2) +
        1.0f;

    return {sin, cos * sign};
}

[[nodiscard]] static inline float lerp(float a, float b, float t) noexcept
{
    return a + ((b - a) * t);
}

[[nodiscard]] static inline float clamp(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }

    if (value > max)
    {
        return max;
    }

    return value;
}

template <typename T = float>
[[nodiscard]] static inline T round(float value)
{
    return static_cast<T>(std::round(value));
}
} // namespace violet::math