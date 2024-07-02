#pragma once

#include "math/types.hpp"
#include <cmath>

namespace violet
{
class euler
{
public:
    [[nodiscard]] static inline float3 from_quaternion(const float4& q)
    {
        float sp = -2.0f * (q[1] * q[2] - q[3] * q[0]);
        if (std::fabs(sp) > 0.9999f)
        {
            return {
                1.570796f * sp,
                std::atan2(-q[0] * q[2] + q[3] * q[1], 0.5f - q[1] * q[1] - q[2] * q[2]),
                0.0f};
        }
        else
        {
            return {
                std::asin(sp),
                std::atan2(q[0] * q[2] + q[3] * q[1], 0.5f - q[0] * q[0] - q[1] * q[1]),
                std::atan2(q[0] * q[1] + q[3] * q[2], 0.5f - q[0] * q[0] - q[2] * q[2])};
        }
    }
};
} // namespace violet