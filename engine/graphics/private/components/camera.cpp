#include "components/camera.hpp"
#include "common/hash.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
camera::camera(renderer* renderer)
    : m_render_pass(nullptr),
      m_back_buffer_index(-1),
      m_framebuffer_dirty(false),
      m_framebuffer(nullptr),
      m_renderer(renderer)
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

    m_parameter = m_renderer->create_parameter(m_renderer->get_parameter_layout("violet camera"));
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

void camera::set_skybox(rhi_image* texture, rhi_sampler* sampler)
{
    m_parameter->set_texture(1, texture, sampler);
}

void camera::set_render_pass(render_pass* render_pass)
{
    m_render_pass = render_pass;
    m_attachments.resize(render_pass->get_attachment_count());
}

void camera::set_attachment(std::size_t index, rhi_image* attachment, bool back_buffer)
{
    m_attachments[index] = attachment;
    m_framebuffer_dirty = true;

    if (back_buffer)
        m_back_buffer_index = index;
    else if (m_back_buffer_index == index)
        m_back_buffer_index = -1;
}

void camera::set_back_buffer(rhi_image* back_buffer)
{
    if (m_back_buffer_index != -1)
    {
        m_attachments[m_back_buffer_index] = back_buffer;
        m_framebuffer_dirty = true;
    }
}

rhi_framebuffer* camera::get_framebuffer()
{
    if (!m_framebuffer_dirty)
        return m_framebuffer;

    std::size_t hash = 0;
    for (rhi_image* attachment : m_attachments)
        hash = hash_combine(hash, attachment->get_hash());

    auto iter = m_framebuffer_cache.find(hash);
    if (iter == m_framebuffer_cache.end())
    {
        rhi_framebuffer_desc desc = {};
        desc.render_pass = m_render_pass->get_interface();

        for (std::size_t i = 0; i < m_attachments.size(); ++i)
            desc.attachments[i] = m_attachments[i];
        desc.attachment_count = m_attachments.size();

        m_framebuffer_cache[hash] = m_renderer->create_framebuffer(desc);
        m_framebuffer = m_framebuffer_cache[hash].get();
    }
    else
    {
        m_framebuffer = iter->second.get();
    }

    m_framebuffer_dirty = false;
    return m_framebuffer;
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

    m_framebuffer_cache.clear();
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
    m_parameter->set_uniform(0, &m_parameter_data, sizeof(camera_parameter), 0);
}
} // namespace violet