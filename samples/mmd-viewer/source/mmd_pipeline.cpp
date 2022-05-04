#include "mmd_pipeline.hpp"

namespace ash::sample::mmd
{
mmd_pass::mmd_pass(graphics::technique_interface* interface) : graphics::technique(interface)
{
}

void mmd_pass::render(const graphics::camera& camera, graphics::render_command_interface* command)
{
    static std::size_t counter = 0;
    command->begin(interface(), m_render_target_sets[counter].get());

    command->parameter(3, camera.parameter->parameter());
    for (auto& unit : units())
    {
        command->parameter(0, unit->parameters[0]->parameter());
        command->parameter(1, unit->parameters[1]->parameter());
        command->parameter(2, unit->parameters[2]->parameter());

        command->draw(
            unit->vertex_buffer,
            unit->index_buffer,
            unit->index_start,
            unit->index_end,
            unit->vertex_base);
    }

    command->end(interface());

    counter = (counter + 1) % m_render_target_sets.size();
}

void mmd_pass::initialize_render_target_set(graphics::graphics& graphics)
{
    auto back_buffers = graphics.back_buffers();
    m_depth_stencil = graphics.make_depth_stencil(1300, 800, 1);

    for (std::size_t i = 0; i < back_buffers.size(); ++i)
    {
        graphics::render_target_set_info render_target_set_info;
        render_target_set_info.render_targets.push_back(back_buffers[i]);
        render_target_set_info.render_targets.push_back(m_depth_stencil.get());
        render_target_set_info.technique = interface();
        render_target_set_info.width = 1300;
        render_target_set_info.height = 800;
        m_render_target_sets.emplace_back(graphics.make_render_target_set(render_target_set_info));
    }
}
} // namespace ash::sample::mmd