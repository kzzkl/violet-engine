#include "graphics/passes/mesh_pass.hpp"

namespace violet
{
mesh_pass::mesh_pass(const data& data)
{
    add_render_target(data.render_target, RHI_ATTACHMENT_LOAD_OP_CLEAR);
    set_depth_stencil(data.depth_buffer, RHI_ATTACHMENT_LOAD_OP_CLEAR);
    set_execute(
        [&data](rdg_command* command)
        {
            rhi_texture_extent extent = data.render_target->get_rhi()->get_extent();
            rhi_viewport viewport = {
                .x = 0,
                .y = 0,
                .width = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height),
                .min_depth = 0.0f,
                .max_depth = 1.0f};
            command->set_viewport(viewport);

            rhi_scissor_rect scissor =
                {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};
            command->set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            command->draw_render_list(data.render_list);
        });
}
} // namespace violet