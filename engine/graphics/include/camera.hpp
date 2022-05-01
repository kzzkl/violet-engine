#pragma once

#include "component.hpp"
#include "math.hpp"
#include "render_parameter.hpp"

namespace ash::graphics
{
struct main_camera
{
};

struct camera
{
    void set(float fov, float aspect, float near_z, float far_z, bool flip_y = false)
    {
        math::float4x4_simd proj = math::matrix_simd::perspective(fov, aspect, near_z, far_z);
        math::simd::store(proj, projection);

        if (flip_y)
            projection[1][1] *= -1.0f;
    }

    math::float4x4 view;
    math::float4x4 projection;

    resource* render_target{nullptr};
    resource* depth_stencil{nullptr};
    std::unique_ptr<render_parameter> parameter;

    std::uint32_t mask{std::numeric_limits<std::uint32_t>::max()};
};
} // namespace ash::graphics