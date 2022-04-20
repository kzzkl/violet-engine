#include "debug_pipeline.hpp"
#include "render_pipeline.hpp"

namespace ash::graphics
{
debug_pipeline::debug_pipeline(
    layout_type* layout,
    pipeline_type* pipeline,
    const std::vector<resource*>& vertex_buffer,
    resource* index_buffer)
    : render_pipeline(layout, pipeline),
      m_index(0)
{
    for (auto buffer : vertex_buffer)
        m_vertex_buffer.emplace_back(buffer);

    m_index_buffer.reset(index_buffer);
}

void debug_pipeline::draw_line(
    const math::float3& start,
    const math::float3& end,
    const math::float3& color)
{
    m_vertics.push_back(vertex{start, color});
    m_vertics.push_back(vertex{end, color});
}

void debug_pipeline::render(resource* target, render_command* command, render_parameter* pass)
{
    m_vertex_buffer[m_index]->upload(m_vertics.data(), sizeof(vertex) * m_vertics.size(), 0);

    command->pipeline(pipeline());
    command->layout(layout());
    command->parameter(0, pass->parameter());
    command->draw(
        m_vertex_buffer[m_index].get(),
        m_index_buffer.get(),
        0,
        m_vertics.size() * 2,
        0,
        primitive_topology_type::LINE_LIST,
        target);

    m_index = (m_index + 1) % m_vertex_buffer.size();
}
} // namespace ash::graphics