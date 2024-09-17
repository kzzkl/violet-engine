#include "graphics/passes/mesh_pass.hpp"

namespace violet
{
mesh_pass::mesh_pass(const data& data)
{
    add_render_target(data.render_target, data.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    set_depth_stencil(data.depth_buffer, data.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    set_execute(
        [&data](rdg_command* command)
        {
            command->set_viewport(data.viewport);

            rhi_scissor_rect scissor = {
                .min_x = 0,
                .min_y = 0,
                .max_x = static_cast<std::uint32_t>(data.viewport.width),
                .max_y = static_cast<std::uint32_t>(data.viewport.height)};
            command->set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            command->draw_render_list(data.render_list);
        });
}
} // namespace violet