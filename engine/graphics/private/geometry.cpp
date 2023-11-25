#include "graphics/geometry.hpp"

namespace violet
{
geometry::geometry(rhi_renderer* rhi) : m_index_buffer(nullptr), m_rhi(rhi)
{
}

geometry::~geometry()
{
    for (auto [key, value] : m_vertex_buffers)
        m_rhi->destroy_buffer(value);
    m_rhi->destroy_buffer(m_index_buffer);
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
    rhi_buffer_flags flags)
{
    if (m_vertex_buffers.find(name.data()) != m_vertex_buffers.end())
        return;

    rhi_buffer_desc desc = {};
    desc.data = data;
    desc.size = size;
    desc.flags = flags;
    m_vertex_buffers[name.data()] = m_rhi->create_buffer(desc);
}

void geometry::set_indices(
    const void* data,
    std::size_t size,
    std::size_t index_size,
    rhi_buffer_flags flags)
{
    if (m_index_buffer != nullptr)
        return;

    rhi_buffer_desc desc = {};
    desc.data = data;
    desc.size = size;
    desc.index.size = index_size;
    desc.flags = flags;
    m_index_buffer = m_rhi->create_buffer(desc);
}
} // namespace violet