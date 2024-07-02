#include "components/camera.hpp"
#include "common/hash.hpp"
#include "graphics/pipeline_parameter.hpp"
#include "graphics/render_device.hpp"
#include <cassert>

namespace violet
{
camera::camera() : m_state(CAMERA_STATE_ENABLE), m_priority(0.0f), m_renderer(nullptr)
{
    m_perspective.fov = 45.0f;
    m_perspective.near_z = 0.1f;
    m_perspective.far_z = 1000.0f;

    m_view = matrix::identity<float4x4>();
    math::store(
        matrix::perspective(
            m_perspective.fov,
            static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
            m_perspective.near_z,
            m_perspective.far_z),
        m_projection);

    m_parameter = render_device::instance().create_parameter(pipeline_parameter_camera);
}

camera::~camera()
{
}

void camera::set_perspective(float fov, float near_z, float far_z)
{
    m_perspective.fov = fov;
    m_perspective.near_z = near_z;
    m_perspective.far_z = far_z;
    update_projection();
}

void camera::set_position(const float3& position)
{
    m_parameter->set_uniform(
        0,
        &position,
        sizeof(float3),
        offsetof(pipeline_parameter_camera_data, position));
}

void camera::set_view(const float4x4& view)
{
    m_view = view;

    matrix4 v = math::load(m_view);
    matrix4 p = math::load(m_projection);

    float4x4 view_projection;
    math::store(matrix::mul(v, p), view_projection);

    m_parameter
        ->set_uniform(0, &m_view, sizeof(float4x4), offsetof(pipeline_parameter_camera_data, view));

    m_parameter->set_uniform(
        0,
        &view_projection,
        sizeof(float4x4),
        offsetof(pipeline_parameter_camera_data, view_projection));
}

void camera::set_skybox(rhi_texture* texture, rhi_sampler* sampler)
{
    m_parameter->set_texture(1, texture, sampler);
}

void camera::resize(std::uint32_t width, std::uint32_t height)
{
    m_scissor.max_x = m_scissor.min_x + width;
    m_scissor.max_y = m_scissor.min_y + height;

    m_viewport.width = width;
    m_viewport.height = height;
    m_viewport.min_depth = 0.0f;
    m_viewport.max_depth = 1.0f;

    update_projection();
}

void camera::set_renderer(renderer* renderer) noexcept
{
    m_renderer = renderer;
}

void camera::set_render_target(std::size_t index, rhi_texture* texture)
{
    if (m_render_targets.size() <= index)
        m_render_targets.resize(index + 1);

    m_render_targets[index].is_swapchain = false;
    m_render_targets[index].texture = texture;
}

void camera::set_render_target(std::size_t index, rhi_swapchain* swapchain)
{
    if (m_render_targets.size() <= index)
        m_render_targets.resize(index + 1);

    m_render_targets[index].is_swapchain = true;
    m_render_targets[index].swapchain = swapchain;
}

std::vector<rhi_texture*> camera::get_render_targets() const
{
    std::vector<rhi_texture*> result;

    for (auto& render_target : m_render_targets)
    {
        rhi_texture* texture = render_target.is_swapchain ? render_target.swapchain->get_texture()
                                                          : render_target.texture;
        result.push_back(texture);
    }

    return result;
}

std::vector<rhi_swapchain*> camera::get_swapchains() const
{
    std::vector<rhi_swapchain*> result;

    for (auto& render_target : m_render_targets)
    {
        if (render_target.is_swapchain)
            result.push_back(render_target.swapchain);
    }

    return result;
}

void camera::update_projection()
{
    math::store(
        matrix::perspective(
            m_perspective.fov,
            static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
            m_perspective.near_z,
            m_perspective.far_z),
        m_projection);

    matrix4 v = math::load(m_view);
    matrix4 p = math::load(m_projection);

    float4x4 view_projection;
    math::store(matrix::mul(v, p), view_projection);

    m_parameter->set_uniform(
        0,
        &m_projection,
        sizeof(float4x4),
        offsetof(pipeline_parameter_camera_data, projection));

    m_parameter->set_uniform(
        0,
        &view_projection,
        sizeof(float4x4),
        offsetof(pipeline_parameter_camera_data, view_projection));
}
} // namespace violet