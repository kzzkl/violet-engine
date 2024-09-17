#pragma once

#include "math/math.hpp"

namespace violet
{
struct transform
{
    float3 position{0.0f, 0.0f, 0.0f};
    float4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
    float3 scale{1.0f, 1.0f, 1.0f};

    void lookat(const float3& target, const float3& up)
    {
        vector4 t = math::load(target);
        vector4 p = math::load(position);
        vector4 u = math::load(up);

        vector4 z_axis = vector::normalize(vector::sub(t, p));
        vector4 x_axis = vector::normalize(vector::cross(u, z_axis));
        vector4 y_axis = vector::cross(z_axis, x_axis);

        matrix4 rotation_matrix = {x_axis, y_axis, z_axis, math::IDENTITY_ROW_3};
        math::store(quaternion::from_matrix(rotation_matrix), rotation);
    }
};

struct transform_local
{
    float4x4 matrix;
};

struct transform_world
{
    float4x4 matrix;

    float3 get_position() const
    {
        return float3{matrix[3][0], matrix[3][1], matrix[3][2]};
    }
};
} // namespace violet