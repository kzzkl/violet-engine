#include "components/camera.hpp"
#include "common/hash.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
camera::camera()
    : m_parameter(nullptr),
      m_render_pass(nullptr),
      m_back_buffer_index(-1),
      m_framebuffer_dirty(false),
      m_framebuffer(nullptr)
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
}

camera::camera(camera&& other) noexcept
{
    m_perspective = other.m_perspective;
    m_parameter_data = other.m_parameter_data;
    m_parameter = other.m_parameter;
    m_render_pass = other.m_render_pass;
    m_scissor = other.m_scissor;
    m_viewport = other.m_viewport;
    m_attachments = std::move(other.m_attachments);
    m_back_buffer_index = other.m_back_buffer_index;
    m_framebuffer_dirty = other.m_framebuffer_dirty;
    m_framebuffer = other.m_framebuffer;
    m_framebuffer_cache = std::move(m_framebuffer_cache);

    other.m_parameter = nullptr;
    other.m_render_pass = nullptr;
    other.m_framebuffer_cache.clear();
}

camera::~camera()
{
    if (m_render_pass)
    {
        rhi_renderer* rhi = m_render_pass->get_context()->get_rhi();
        if (m_parameter != nullptr)
            rhi->destroy_parameter(m_parameter);

        for (auto [hash, framebuffer] : m_framebuffer_cache)
            rhi->destroy_framebuffer(framebuffer);
    }
}

void camera::set_perspective(float fov, float near_z, float far_z)
{
    m_perspective.fov = fov;
    m_perspective.near_z = near_z;
    m_perspective.far_z = far_z;
    update_projection();
}

void camera::set_view(const float4x4& view)
{
    m_parameter_data.view = view;
    update_parameter();
}

void camera::set_render_pass(render_pass* render_pass)
{
    m_render_pass = render_pass;
    m_attachments.resize(render_pass->get_attachment_count());

    if (m_parameter == nullptr)
    {
        rhi_renderer* rhi = render_pass->get_context()->get_rhi();
        rhi_parameter_layout* layout =
            render_pass->get_context()->get_parameter_layout("violet camera");
        m_parameter = rhi->create_parameter(layout);
    }
}

void camera::set_attachment(std::size_t index, rhi_resource* attachment, bool back_buffer)
{
    m_attachments[index] = attachment;
    m_framebuffer_dirty = true;

    if (back_buffer)
        m_back_buffer_index = index;
    else if (m_back_buffer_index == index)
        m_back_buffer_index = -1;
}

void camera::set_back_buffer(rhi_resource* back_buffer)
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
    for (rhi_resource* attachment : m_attachments)
        hash = hash_combine(hash, attachment->get_hash());

    auto iter = m_framebuffer_cache.find(hash);
    if (iter == m_framebuffer_cache.end())
    {
        rhi_framebuffer_desc desc = {};
        desc.render_pass = m_render_pass->get_interface();

        for (std::size_t i = 0; i < m_attachments.size(); ++i)
            desc.attachments[i] = m_attachments[i];
        desc.attachment_count = m_attachments.size();

        m_framebuffer = m_render_pass->get_context()->get_rhi()->create_framebuffer(desc);
        m_framebuffer_cache[hash] = m_framebuffer;
    }
    else
    {
        m_framebuffer = iter->second;
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

camera& camera::operator=(camera&& other) noexcept
{
    m_perspective = other.m_perspective;
    m_parameter_data = other.m_parameter_data;
    m_parameter = other.m_parameter;
    m_render_pass = other.m_render_pass;
    m_scissor = other.m_scissor;
    m_viewport = other.m_viewport;
    m_attachments = std::move(other.m_attachments);
    m_back_buffer_index = other.m_back_buffer_index;
    m_framebuffer_dirty = other.m_framebuffer_dirty;
    m_framebuffer = other.m_framebuffer;
    m_framebuffer_cache = std::move(m_framebuffer_cache);

    other.m_parameter = nullptr;
    other.m_render_pass = nullptr;
    other.m_framebuffer_cache.clear();

    return *this;
}

void camera::update_projection()
{
    m_parameter_data.projection = matrix::perspective(
        m_perspective.fov,
        static_cast<float>(m_viewport.width) / static_cast<float>(m_viewport.height),
        m_perspective.near_z,
        m_perspective.far_z);

    update_parameter();
}

void camera::update_parameter()
{
    float4x4_simd v = simd::load(m_parameter_data.view);
    float4x4_simd p = simd::load(m_parameter_data.projection);
    simd::store(matrix_simd::mul(v, p), m_parameter_data.view_projection);

    m_parameter->set_uniform(0, &m_parameter_data, sizeof(camera_parameter), 0);
}
} // namespace violet