#include "graphics/camera.hpp"
#include "graphics/rhi.hpp"
#include "graphics/visual.hpp"

namespace ash::graphics
{
camera_pipeline_parameter::camera_pipeline_parameter() : pipeline_parameter("ash_camera")
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
    field<constant_data>(0).view = math::matrix_plain::transpose(view);
}

void camera_pipeline_parameter::projection(const math::float4x4& projection)
{
    field<constant_data>(0).projection = math::matrix_plain::transpose(projection);
}

void camera_pipeline_parameter::view_projection(const math::float4x4& view_projection)
{
    field<constant_data>(0).view_projection = math::matrix_plain::transpose(view_projection);
}

std::vector<pipeline_parameter_pair> camera_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER,
         sizeof(camera_pipeline_parameter::constant_data)}
    };
}

camera::camera() noexcept
    : m_fov(45.0f),
      m_near_z(0.3f),
      m_far_z(1000.0f),
      m_flip_y(false),
      m_projection(math::matrix_plain::identity()),
      m_render_target(nullptr),
      m_render_target_resolve(nullptr),
      m_depth_stencil_buffer(nullptr),
      mask(VISUAL_GROUP_1 | VISUAL_GROUP_UI)
{
    m_parameter = std::make_unique<camera_pipeline_parameter>();
}

void camera::field_of_view(float fov) noexcept
{
    m_fov = fov;
    update_projection();
}

void camera::clipping_planes(float near_z, float far_z) noexcept
{
    m_near_z = near_z;
    m_far_z = far_z;
    update_projection();
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
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    math::float4x4_simd proj = math::matrix_simd::perspective(m_fov, aspect, m_near_z, m_far_z);
    math::simd::store(proj, m_projection);

    if (m_flip_y)
        m_projection[1][1] *= -1.0f;
}
} // namespace ash::graphics