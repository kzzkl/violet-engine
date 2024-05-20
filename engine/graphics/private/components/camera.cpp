#include "components/camera.hpp"
#include "common/hash.hpp"
#include "graphics/render_device.hpp"
#include <cassert>

namespace violet
{
camera::camera(render_device* device)
    : m_state(CAMERA_STATE_ENABLE),
      m_priority(0.0f),
      m_render_graph(nullptr)
{
    m_perspective.fov = 45.0f;
    m_perspective.near_z = 0.1f;
    m_perspective.far_z = 1000.0f;

    m_parameter_data.view = matrix::identity();
    m_parameter_data.projection = matrix::perspective(
        m_perspective.fov,
        static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
        m_perspective.near_z,
        m_perspective.far_z);

    m_parameter = device->create_parameter(engine_parameter_layout::camera);
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
    m_parameter_data.positon = position;
    update_parameter();
}

void camera::set_view(const float4x4& view)
{
    m_parameter_data.view = view;

    float4x4_simd v = simd::load(m_parameter_data.view);
    float4x4_simd p = simd::load(m_parameter_data.projection);
    simd::store(matrix_simd::mul(v, p), m_parameter_data.view_projection);

    update_parameter();
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

void camera::set_render_graph(render_graph* render_graph) noexcept
{
    m_render_graph = render_graph;
    m_render_context = render_graph->create_context();
    m_render_context->set_camera(m_parameter.get());
}

void camera::set_render_texture(std::string_view name, rhi_texture* texture)
{
    std::size_t index = m_render_graph->get_resource_index(name);
    assert(m_render_graph->get_resource_type(index) == RDG_RESOURCE_TYPE_TEXTURE);
    m_render_context->set_texture(index, texture);
}

void camera::set_render_texture(std::string_view name, rhi_swapchain* swapchain)
{
    std::size_t index = m_render_graph->get_resource_index(name);
    assert(m_render_graph->get_resource_type(index) == RDG_RESOURCE_TYPE_TEXTURE);

    auto iter = std::find_if(
        m_swapchains.begin(),
        m_swapchains.end(),
        [index](auto& pair)
        {
            return pair.second == index;
        });
    if (iter != m_swapchains.end())
        iter->first = swapchain;
    else
        m_swapchains.push_back({swapchain, index});
}

void camera::update_projection()
{
    m_parameter_data.projection = matrix::perspective(
        m_perspective.fov,
        static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
        m_perspective.near_z,
        m_perspective.far_z);

    float4x4_simd v = simd::load(m_parameter_data.view);
    float4x4_simd p = simd::load(m_parameter_data.projection);
    simd::store(matrix_simd::mul(v, p), m_parameter_data.view_projection);

    update_parameter();
}

void camera::update_parameter()
{
    m_parameter->set_uniform(0, &m_parameter_data, sizeof(parameter_data), 0);
}
} // namespace violet