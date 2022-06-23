#include "graphics/camera.hpp"
#include "graphics/rhi.hpp"
#include "graphics/visual.hpp"

namespace ash::graphics
{
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
    m_parameter = rhi::make_pipeline_parameter("ash_pass");
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