#include "graphics/geometry.hpp"

namespace violet
{
geometry::geometry()
{
}

geometry::~geometry()
{
}

rhi_buffer* geometry::get_vertex_buffer(std::string_view name)
{
    auto iter = m_vertex_buffers.find(name.data());
    if (iter != m_vertex_buffers.end())
        return iter->second.get();
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
    m_vertex_buffers[name.data()] = render_device::instance().create_buffer(desc);
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
    m_index_buffer = render_device::instance().create_buffer(desc);
}
} // namespace violet