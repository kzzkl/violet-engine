#pragma once

#include "ecs/component.hpp"
#include "math/math.hpp"

namespace ash::graphics
{
class resource_interface;
class pipeline_parameter_interface;
class camera
{
public:
    camera() noexcept;

    void field_of_view(float fov) noexcept;
    void clipping_planes(float near_z, float far_z) noexcept;
    void flip_y(bool flip) noexcept;

    void render_target(resource_interface* render_target) noexcept;
    void depth_stencil_buffer(resource_interface* depth_stencil_buffer) noexcept;
    void render_target_resolve(resource_interface* render_target_resolve) noexcept;

    inline resource_interface* render_target() const noexcept { return m_render_target; }
    inline resource_interface* render_target_resolve() const noexcept
    {
        return m_render_target_resolve;
    }
    inline resource_interface* depth_stencil_buffer() const noexcept
    {
        return m_depth_stencil_buffer;
    }

    const math::float4x4& projection() const noexcept { return m_projection; }
    pipeline_parameter_interface* parameter() const noexcept { return m_parameter.get(); }

    std::uint32_t mask;

private:
    void update_projection() noexcept;

    float m_fov;
    float m_near_z;
    float m_far_z;
    bool m_flip_y;
    math::float4x4 m_projection;

    resource_interface* m_render_target;
    resource_interface* m_render_target_resolve;
    resource_interface* m_depth_stencil_buffer;

    std::unique_ptr<pipeline_parameter_interface> m_parameter;
};
} // namespace ash::graphics