#include "graphics/geometry.hpp"

namespace violet
{
geometry::geometry(rhi_renderer* rhi) : m_index_buffer(nullptr), m_rhi(rhi)
{
}

geometry::~geometry()
{
    for (auto [key, value] : m_vertex_buffers)
        m_rhi->destroy_vertex_buffer(value);
    m_rhi->destroy_index_buffer(m_index_buffer);
}

rhi_resource* geometry::get_vertex_buffer(std::string_view name)
{
    auto iter = m_vertex_buffers.find(name.data());
    if (iter != m_vertex_buffers.end())
        return iter->second;
    else
        return nullptr;
}

void geometry::add_attribute(
    std::string_view name,
    const void* data,
    std::size_t size,
    bool dynamic)
{
    if (m_vertex_buffers.find(name.data()) != m_vertex_buffers.end())
        return;

    rhi_vertex_buffer_desc desc = {};
    desc.data = data;
    desc.size = size;
    desc.dynamic = dynamic;
    m_vertex_buffers[name.data()] = m_rhi->create_vertex_buffer(desc);
}

void geometry::set_indices(const void* data, std::size_t size, std::size_t index_size, bool dynamic)
{
    if (m_index_buffer != nullptr)
        return;

    rhi_index_buffer_desc desc = {};
    desc.data = data;
    desc.size = size;
    desc.index_size = index_size;
    desc.dynamic = dynamic;
    m_index_buffer = m_rhi->create_index_buffer(desc);
}
} // namespace violet