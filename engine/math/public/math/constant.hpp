#pragma once

#include "math/type.hpp"

namespace violet
{
struct math
{
    static constexpr float PI = 3.141592654f;
    static constexpr float PI_2PI = 2.0f * PI;
    static constexpr float PI_1DIVPI = 1.0f / PI;
    static constexpr float PI_1DIV2PI = 1.0f / PI_2PI;
    static constexpr float PI_PIDIV2 = PI / 2.0f;
    static constexpr float PI_PIDIV4 = PI / 4.0f;
    static constexpr float PI_PIDIV180 = PI / 180.0f;
    static constexpr float PI_180DIVPI = 180.0f / PI;

#ifdef VIOLET_USE_SIMD
    static constexpr simd::convert<float> identity_row_0 = {1.0f, 0.0f, 0.0f, 0.0f};
    static constexpr simd::convert<float> identity_row_1 = {0.0f, 1.0f, 0.0f, 0.0f};
    static constexpr simd::convert<float> identity_row_2 = {0.0f, 0.0f, 1.0f, 0.0f};
    static constexpr simd::convert<float> identity_row_3 = {0.0f, 0.0f, 0.0f, 1.0f};
#else
    static constexpr matrix4::row_type identity_row_0 = {1.0f, 0.0f, 0.0f, 0.0f};
    static constexpr matrix4::row_type identity_row_1 = {0.0f, 1.0f, 0.0f, 0.0f};
    static constexpr matrix4::row_type identity_row_2 = {0.0f, 0.0f, 1.0f, 0.0f};
    static constexpr matrix4::row_type identity_row_3 = {0.0f, 0.0f, 0.0f, 1.0f};
#endif
};
} // namespace violet