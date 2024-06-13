#pragma once

#include "math/constant.hpp"
#include <utility>

namespace violet
{
[[nodiscard]] inline float to_radians(float degrees)
{
    return degrees * math::PI_PIDIV180;
}

[[nodiscard]] inline float to_degrees(float radians)
{
    return radians * math::PI_180DIVPI;
}

[[nodiscard]] inline std::pair<float, float> sin_cos(float radians)
{
    float temp = radians * math::PI_1DIV2PI;
    if (temp > 0.0f)
        temp = static_cast<float>(static_cast<int>(temp + 0.5f));
    else
        temp = static_cast<float>(static_cast<int>(temp - 0.5f));

    float x = radians - math::PI_2PI * temp;

    float sign = 1.0f;
    if (x > math::PI_PIDIV2)
    {
        x = math::PI - x;
        sign = -1.0f;
    }
    else if (x < -math::PI_PIDIV2)
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
        ((((-2.6051615e-07f * x2 + 2.4760495e-05f) * x2 - 0.0013888378f) * x2 + 0.041666638f) * x2 -
         0.5f) *
            x2 +
        1.0f;

    return {sin, cos * sign};
}

[[nodiscard]] inline float clamp(float value, float min, float max)
{
    if (value < min)
        return min;
    else if (value > max)
        return max;
    else
        return value;
}
} // namespace violet