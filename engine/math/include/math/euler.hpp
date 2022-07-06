#pragma once

#include "type.hpp"
#include <cmath>

namespace ash::math
{
class euler
{
public:
    using euler_type = float3;
    using quaternion_type = float4;

public:
    static inline euler_type rotation_quaternion(const quaternion_type& q)
    {
        float sp = -2.0f * (q[1] * q[2] - q[3] * q[0]);
        if (std::fabs(sp) > 0.9999f)
        {
            return {
                std::atan2(-q[0] * q[2] + q[3] * q[1], 0.5f - q[1] * q[1] - q[2] * q[2]),
                1.570796f * sp,
                0.0f};
        }
        else
        {
            return {
                std::atan2(q[0] * q[2] + q[3] * q[1], 0.5f - q[0] * q[0] - q[1] * q[1]),
                std::asin(sp),
                std::atan2(q[0] * q[1] + q[3] * q[2], 0.5f - q[0] * q[0] - q[2] * q[2])};
        }
    }
};
} // namespace ash::math