#pragma once

#include "component.hpp"
#include "math.hpp"

namespace ash::graphics
{
struct main_camera
{
};

struct camera
{
    void set(float fov, float aspect, float near_z, float far_z)
    {
        math::float4x4_simd proj = math::matrix_simd::perspective(fov, aspect, near_z, far_z);
        math::simd::store(proj, projection);
    }

    math::float4x4 view;
    math::float4x4 projection;
};
} // namespace ash::graphics