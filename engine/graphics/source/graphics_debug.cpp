#include "graphics_debug.hpp"
#include "render_pipeline.hpp"

namespace ash::graphics
{
graphics_debug::graphics_debug(const std::vector<resource*>& vertex_buffer, resource* index_buffer)
    : m_index(0)
{
    for (auto buffer : vertex_buffer)
        m_vertex_buffer.emplace_back(buffer);

    m_index_buffer.reset(index_buffer);
}

void graphics_debug::draw_line(
    const math::float3& start,
    const math::float3& end,
    const math::float3& color)
{
    m_vertics.push_back(vertex{start, color});
    m_vertics.push_back(vertex{end, color});
}

void graphics_debug::sync()
{
    m_vertex_buffer[m_index]->upload(m_vertics.data(), sizeof(vertex) * m_vertics.size(), 0);
    m_index = (m_index + 1) % m_vertex_buffer.size();
}
} // namespace ash::graphics