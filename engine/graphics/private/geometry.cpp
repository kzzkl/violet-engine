#include "graphics/geometry.hpp"

namespace violet
{
geometry::geometry() = default;

geometry::~geometry() {}

void geometry::add_attribute(
    std::string_view name,
    const void* data,
    std::size_t size,
    rhi_buffer_flags flags)
{
    assert(m_vertex_buffer_map.find(name.data()) == m_vertex_buffer_map.end());

    rhi_buffer_desc desc = {
        .data = data,
        .size = size,
        .flags = flags,
    };

    auto vertex_buffer = render_device::instance().create_buffer(desc);
    m_vertex_buffer_map[name.data()] = vertex_buffer.get();
    m_buffers.push_back(std::move(vertex_buffer));
}

void geometry::add_attribute(std::string_view name, rhi_buffer* buffer)
{
    assert(m_vertex_buffer_map.find(name.data()) == m_vertex_buffer_map.end());
    m_vertex_buffer_map[name.data()] = buffer;
}

rhi_buffer* geometry::get_vertex_buffer(std::string_view name) const
{
    auto iter = m_vertex_buffer_map.find(name.data());
    if (iter != m_vertex_buffer_map.end())
    {
        return iter->second;
    }

    return nullptr;
}

void geometry::set_indexes(
    const void* data,
    std::size_t size,
    std::size_t index_size,
    rhi_buffer_flags flags)
{
    assert(m_index_buffer == nullptr);

    rhi_buffer_desc desc = {
        .data = data,
        .size = size,
        .flags = flags,
        .index =
            {
                .size = index_size,
            },
    };

    auto index_buffer = render_device::instance().create_buffer(desc);
    m_index_buffer = index_buffer.get();
    m_buffers.push_back(std::move(index_buffer));
}

void geometry::set_indexes(rhi_buffer* buffer)
{
    assert(m_index_buffer == nullptr);

    m_index_buffer = buffer;
}
} // namespace violet