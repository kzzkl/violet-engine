#pragma once

#include <cmath>

namespace ash::math
{
static constexpr float PI = 3.141592654f;
static constexpr float PI_1_DIV_PI = 0.318309886f;
static constexpr float PI_1_DIV_2PI = 0.159154943f;
static constexpr float PI_2PI = 6.283185307f;
static constexpr float PI_DIV_2 = 1.570796327f;
static constexpr float PI_DIV_4 = 0.785398163f;
static constexpr float PI_DIV_180 = 0.017453292522f;

inline float to_radians(float degrees)
{
    return degrees * PI_DIV_180;
}

inline void sin_cos(float radians, float& sinv, float& cosv)
{
    // TODO: optimization
    sinv = sin(radians);
    cosv = cos(radians);
}
} // namespace ash::math