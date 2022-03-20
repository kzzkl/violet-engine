#pragma once

#include <utility>

namespace ash::math
{
static constexpr float PI = 3.141592654f;
static constexpr float PI_1DIVPI = 0.318309886f;
static constexpr float PI_1DIV2PI = 0.159154943f;
static constexpr float PI_2PI = 6.283185307f;
static constexpr float PI_PIDIV2 = 1.570796327f;
static constexpr float PI_PIDIV4 = 0.785398163f;
static constexpr float DIV180 = 0.017453292522f;

inline float to_radians(float degrees)
{
    return degrees * DIV180;
}

inline std::pair<float, float> sin_cos(float radians)
{
    float temp = radians * PI_1DIV2PI;
    if (temp > 0.0f)
        temp = static_cast<float>(static_cast<int>(temp + 0.5f));
    else
        temp = static_cast<float>(static_cast<int>(temp - 0.5f));

    float x = radians - PI_2PI * temp;

    float sign = 1.0f;
    if (x > PI_PIDIV2)
    {
        x = PI - x;
        sign = -1.0f;
    }
    else if (x < -PI_PIDIV2)
    {
        x = -PI - x;
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
} // namespace ash::math