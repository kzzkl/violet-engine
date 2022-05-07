#include "ui_pipeline.hpp"

namespace ash::ui
{
ui_pass::ui_pass(graphics::render_pass_interface* interface) : render_pass(interface)
{
}

void ui_pass::render(const graphics::camera& camera, graphics::render_command_interface* command)
{
    /*command->pipeline(pipeline());
    command->layout(layout());
    command->render_target(target, depth_stencil);

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
    }*/
}
} // namespace ash::ui