#include "graphics/geometry.hpp"

namespace violet
{
geometry::geometry() = default;

geometry::~geometry() {}

void geometry::add_attribute(std::string_view name, const rhi_buffer_desc& desc)
{
    assert(m_vertex_buffer_map.find(name.data()) == m_vertex_buffer_map.end());

    auto buffer = std::make_unique<vertex_buffer>(desc);
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