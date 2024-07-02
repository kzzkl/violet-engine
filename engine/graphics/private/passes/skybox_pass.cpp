#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
skybox_pass::skybox_pass()
{
}

void skybox_pass::execute(rhi_command* command, rdg_context* context)
{
    rhi_texture_extent extent = context->get_texture(this, reference_render_target)->get_extent();

    rhi_viewport viewport = {};
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    command->set_viewport(viewport);

    rhi_scissor_rect scissor = {};
    scissor.max_x = extent.width;
    scissor.max_y = extent.height;
    command->set_scissor(&scissor, 1);

    // command->set_render_pipeline(get_pipeline());

    command->set_render_parameter(0, context->get_camera());
    command->draw(0, 36);
}
} // namespace violet