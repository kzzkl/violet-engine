#pragma once

#include "graphics/pipeline_parameter.hpp"
#include "graphics/render_group.hpp"

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

    std::array<math::float4, 8> frustum_vertices(std::size_t index) const noexcept;

    static std::vector<pipeline_parameter_pair> layout();

private:
    struct constant_data
    {
        math::float3 position;
        float _padding_0;
        math::float3 direction;
        float _padding_1;
        math::float4x4 view;
        math::float4x4 projection;
        math::float4x4 view_projection;
    };
};

class camera
{
public:
    camera() noexcept;

    void perspective(float fov, float near_z, float far_z);
    void orthographic(float width, float near_z, float far_z);

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

    std::uint32_t render_groups;

private:
    void update_projection() noexcept;

    enum projection_type
    {
        PROJECTION_TYPE_PERSPECTIVE,
        PROJECTION_TYPE_ORTHOGRAPHIC
    };

    projection_type m_projection_type;
    union {
        struct
        {
            float fov;
            float near_z;
            float far_z;
        } perspective;

        struct
        {
            float width;
            float near_z;
            float far_z;
        } orthographic;
    } m_data;

    bool m_flip_y;
    math::float4x4 m_projection;

    std::vector<float> m_frustum_divide;
    std::vector<math::float3> m_frustum_corners;

    resource_interface* m_render_target;
    resource_interface* m_render_target_resolve;
    resource_interface* m_depth_stencil_buffer;

    std::unique_ptr<camera_pipeline_parameter> m_parameter;
};
} // namespace ash::graphics