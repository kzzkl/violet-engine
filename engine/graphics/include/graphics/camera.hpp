#pragma once

#include "graphics/pipeline_parameter.hpp"

namespace ash::graphics
{
class camera_pipeline_parameter : public pipeline_parameter
{
public:
public:
    camera_pipeline_parameter();

    void position(const math::float3& position);
    void direction(const math::float3& direction);
    void view(const math::float4x4& view);
    void projection(const math::float4x4& projection);
    void view_projection(const math::float4x4& view_projection);

    static std::vector<pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        math::float3 position;
        float _padding_1;
        math::float3 direction;
        float _padding_2;
        math::float4x4 view;
        math::float4x4 projection;
        math::float4x4 view_projection;
    };
};

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

    camera_pipeline_parameter* pipeline_parameter() const noexcept { return m_parameter.get(); }

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

    std::unique_ptr<camera_pipeline_parameter> m_parameter;
};
} // namespace ash::graphics