#include "ui_pipeline.hpp"

namespace ash::ui
{
ui_pipeline::ui_pipeline(graphics::pipeline_layout* layout, graphics::pipeline* pipeline)
    : render_pipeline(layout, pipeline)
{
}

void ui_pipeline::render(
    graphics::resource* target,
    graphics::render_command* command,
    graphics::render_parameter* pass)
{
    command->pipeline(pipeline());
    command->layout(layout());

    for (auto& unit : units())
    {
        for (std::size_t i = 0; i < unit_parameter_count(); ++i)
            command->parameter(i, unit->parameters[i]->parameter());

        auto rect = static_cast<graphics::scissor_rect*>(unit->external);
        command->scissor(*rect);
        command->draw(
            unit->vertex_buffer,
            unit->index_buffer,
            unit->index_start,
            unit->index_end,
            unit->vertex_base,
            graphics::primitive_topology_type::TRIANGLE_LIST,
            target);
    }
}
} // namespace ash::ui