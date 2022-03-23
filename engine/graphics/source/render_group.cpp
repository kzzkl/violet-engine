#include "render_group.hpp"

namespace ash::graphics
{
render_group::render_group(
    layout_type* layout,
    pipeline_type* pipeline,
    std::size_t unit_parameter_count,
    std::size_t group_parameter_count)
    : m_layout(layout),
      m_pipeline(pipeline),
      m_parameters(group_parameter_count),
      m_parameter_offset(unit_parameter_count)
{
}

void render_group::render(render_command* command, resource* target)
{
    command->pipeline(m_pipeline.get());
    command->layout(m_layout.get());

    for (std::size_t i = 0; i < m_parameters.size(); ++i)
        command->parameter(m_parameter_offset + i, m_parameters[i]->parameter());

    for (auto [mesh, visual] : m_units)
    {
        for (std::size_t i = 0; i < visual->parameters.size(); ++i)
            command->parameter(i, visual->parameters[i]->parameter());

        command->draw(
            mesh->vertex_buffer.get(),
            mesh->index_buffer.get(),
            primitive_topology_type::TRIANGLE_LIST,
            target);
    }
}

void render_group::parameter(std::size_t index, render_parameter_base* parameter)
{
    m_parameters[index] = parameter;
}
} // namespace ash::graphics