#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
skybox_pass::skybox_pass()
{
    m_color = add_color("color", RHI_TEXTURE_LAYOUT_RENDER_TARGET);
    add_depth_stencil("depth", RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

    set_shader("engine/shaders/skybox.vert.spv", "engine/shaders/skybox.frag.spv");
    set_parameter_layout({engine_parameter_layout::camera});
}

void skybox_pass::execute(rhi_command* command, rdg_context* context)
{
    rhi_texture_extent extent = context->get_texture(m_color->resource->get_index())->get_extent();

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

    command->set_render_pipeline(get_pipeline());

    command->set_render_parameter(0, context->get_camera());
    command->draw(0, 36);
}
} // namespace violet