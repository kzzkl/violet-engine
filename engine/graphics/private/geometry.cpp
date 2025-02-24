#include "graphics/geometry.hpp"

namespace violet
{
geometry::geometry() = default;

geometry::~geometry() {}

void geometry::add_attribute(
    std::string_view name,
    std::size_t vertex_size,
    std::size_t vertex_count,
    rhi_buffer_flags flags)
{
    assert(vertex_size != 0 && vertex_count != 0);

    auto buffer = std::make_unique<vertex_buffer>(vertex_size * vertex_count, flags);
    m_vertex_buffer_map[name.data()] = buffer.get();
    m_buffers.emplace_back(std::move(buffer));
}

void geometry::add_attribute(std::string_view name, vertex_buffer* buffer)
{
    assert(m_vertex_buffer_map.find(name.data()) == m_vertex_buffer_map.end());
    m_vertex_buffer_map[name.data()] = buffer;
}

vertex_buffer* geometry::get_vertex_buffer(std::string_view name) const
{
    auto iter = m_vertex_buffer_map.find(name.data());
    if (iter != m_vertex_buffer_map.end())
    {
        return iter->second;
    }

    return nullptr;
}

void geometry::set_indexes(std::size_t index_size, std::size_t index_count, rhi_buffer_flags flags)
{
    assert(index_size != 0 && index_count != 0);
    assert(m_index_buffer == nullptr);

    auto buffer = std::make_unique<index_buffer>(index_size * index_count, index_size, flags);
    m_index_buffer = buffer.get();
    m_buffers.emplace_back(std::move(buffer));
}

void geometry::set_indexes(index_buffer* buffer)
{
    assert(m_index_buffer == nullptr);
    m_index_buffer = buffer;
}

void geometry::add_morph_target(std::string_view name, const std::vector<morph_element>& elements)
{
    if (m_morph_target_buffer == nullptr)
    {
        m_morph_target_buffer = std::make_unique<morph_target_buffer>();
    }

    m_morph_name_to_index[name.data()] = m_morph_target_buffer->get_morph_target_count();

    m_morph_target_buffer->add_morph_target(elements);
}
} // namespace violet