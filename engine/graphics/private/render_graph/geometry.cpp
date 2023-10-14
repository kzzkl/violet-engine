#include "graphics/render_graph/geometry.hpp"

namespace violet
{
geometry::geometry(render_context* context) : render_node(context), m_index_buffer(nullptr)
{
}

geometry::~geometry()
{
    rhi_renderer* rhi = get_context()->get_rhi();

    for (auto [key, value] : m_vertex_buffers)
        rhi->destroy_vertex_buffer(value);
    rhi->destroy_index_buffer(m_index_buffer);
}

rhi_resource* geometry::get_vertex_buffer(std::string_view name)
{
    auto iter = m_vertex_buffers.find(name.data());
    if (iter != m_vertex_buffers.end())
        return iter->second;
    else
        return nullptr;
}

void geometry::add_attribute(std::string_view name, const void* data, std::size_t size)
{
    if (m_vertex_buffers.find(name.data()) != m_vertex_buffers.end())
        return;

    rhi_vertex_buffer_desc desc = {};
    desc.data = data;
    desc.size = size;
    m_vertex_buffers[name.data()] = get_context()->get_rhi()->create_vertex_buffer(desc);
}

void geometry::set_indices(const void* data, std::size_t size, std::size_t index_size)
{
    if (m_index_buffer != nullptr)
        return;

    rhi_index_buffer_desc desc = {};
    desc.data = data;
    desc.size = size;
    desc.index_size = index_size;
    m_index_buffer = get_context()->get_rhi()->create_index_buffer(desc);
}
} // namespace violet