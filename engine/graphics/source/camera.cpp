#include "graphics/camera.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"

namespace violet::graphics
{
camera_pipeline_parameter::camera_pipeline_parameter() : pipeline_parameter("violet_camera")
{
}

void camera_pipeline_parameter::position(const math::float3& position)
{
    field<constant_data>(0).position = position;
}

void camera_pipeline_parameter::direction(const math::float3& direction)
{
    field<constant_data>(0).direction = direction;
}

void camera_pipeline_parameter::view(const math::float4x4& view)
{
    field<constant_data>(0).view = math::matrix::transpose(view);
}

void camera_pipeline_parameter::projection(const math::float4x4& projection)
{
    field<constant_data>(0).projection = math::matrix::transpose(projection);
}

void camera_pipeline_parameter::view_projection(const math::float4x4& view_projection)
{
    field<constant_data>(0).view_projection = math::matrix::transpose(view_projection);
}

std::vector<pipeline_parameter_pair> camera_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER,
         sizeof(camera_pipeline_parameter::constant_data)}
    };
}

camera::camera() noexcept
    : m_flip_y(false),
      m_projection(math::matrix::identity()),
      m_render_target(nullptr),
      m_render_target_resolve(nullptr),
      m_depth_stencil_buffer(nullptr),
      render_groups(RENDER_GROUP_1 | RENDER_GROUP_UI)
{
    m_projection_type = PROJECTION_TYPE_PERSPECTIVE;
    m_data.perspective.fov = 45.0f;
    m_data.perspective.near_z = 0.3f;
    m_data.perspective.far_z = 1000.0f;

    m_parameter = std::make_unique<camera_pipeline_parameter>();
}

void camera::perspective(float fov, float near_z, float far_z)
{
    m_projection_type = PROJECTION_TYPE_PERSPECTIVE;
    m_data.perspective.fov = fov;
    m_data.perspective.near_z = near_z;
    m_data.perspective.far_z = far_z;

    update_projection();
}

void camera::orthographic(float width, float near_z, float far_z)
{
    m_projection_type = PROJECTION_TYPE_ORTHOGRAPHIC;
    m_data.orthographic.width = width;
    m_data.orthographic.near_z = near_z;
    m_data.orthographic.far_z = far_z;

    update_projection();
}

float camera::near_z() const noexcept
{
    if (m_projection_type == PROJECTION_TYPE_PERSPECTIVE)
        return m_data.perspective.near_z;
    else
        return m_data.orthographic.near_z;
}

float camera::far_z() const noexcept
{
    if (m_projection_type == PROJECTION_TYPE_PERSPECTIVE)
        return m_data.perspective.far_z;
    else
        return m_data.orthographic.far_z;
}

void camera::flip_y(bool flip) noexcept
{
    m_flip_y = flip;
    update_projection();
}

void camera::render_target(resource_interface* render_target) noexcept
{
    m_render_target = render_target;
    update_projection();
}

void camera::depth_stencil_buffer(resource_interface* depth_stencil_buffer) noexcept
{
    m_depth_stencil_buffer = depth_stencil_buffer;
}

void camera::render_target_resolve(resource_interface* render_target_resolve) noexcept
{
    m_render_target_resolve = render_target_resolve;
}

void camera::update_projection() noexcept
{
    if (m_render_target == nullptr)
        return;

    auto extent = m_render_target->extent();
    if (m_projection_type == PROJECTION_TYPE_PERSPECTIVE)
    {
        float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);
        math::float4x4_simd proj = math::matrix_simd::perspective(
            m_data.perspective.fov,
            aspect,
            m_data.perspective.near_z,
            m_data.perspective.far_z);
        math::simd::store(proj, m_projection);
    }
    else
    {
        math::float4x4_simd proj = math::matrix_simd::orthographic(
            m_data.orthographic.width,
            m_data.orthographic.width / extent.width * extent.height,
            m_data.orthographic.near_z,
            m_data.orthographic.far_z);
        math::simd::store(proj, m_projection);
    }

    if (m_flip_y)
        m_projection[1][1] *= -1.0f;
}
} // namespace violet::graphics