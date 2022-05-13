#pragma once

#include "component.hpp"
#include "math.hpp"
#include "render_parameter.hpp"

namespace ash::graphics
{
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

    std::unique_ptr<resource> render_target;
    std::unique_ptr<resource> depth_stencil_buffer;
    resource* back_buffer;

    std::unique_ptr<pipeline_parameter> parameter;

    std::uint32_t mask{std::numeric_limits<std::uint32_t>::max()};
};
} // namespace ash::graphics