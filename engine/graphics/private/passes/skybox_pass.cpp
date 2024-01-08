#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
skybox_pass::skybox_pass(renderer* renderer, setup_context& context)
    : render_pass(renderer, context)
{
    pass_slot* render_target = add_slot("render target", RENDER_RESOURCE_TYPE_INPUT_OUTPUT);
    render_target->set_input_layout(RHI_IMAGE_LAYOUT_RENDER_TARGET);

    pass_slot* depth_buffer = add_slot("depth buffer", RENDER_RESOURCE_TYPE_INPUT_OUTPUT);
    depth_buffer->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
    depth_buffer->set_input_layout(RHI_IMAGE_LAYOUT_DEPTH_STENCIL);

    render_subpass* subpass = add_subpass();
    subpass->add_reference(
        render_target,
        RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
        RHI_IMAGE_LAYOUT_RENDER_TARGET);
    subpass->add_reference(
        depth_buffer,
        RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
        RHI_IMAGE_LAYOUT_DEPTH_STENCIL);

    m_pipeline = add_pipeline(subpass);
    m_pipeline->set_shader("engine/shaders/skybox.vert.spv", "engine/shaders/skybox.frag.spv");
    m_pipeline->set_cull_mode(RHI_CULL_MODE_BACK);
    m_pipeline->set_parameter_layouts({renderer->get_parameter_layout("violet camera")});
}

void skybox_pass::execute(execute_context& context)
{
    rhi_render_command* command = context.get_command();

    command->begin(get_interface(), get_framebuffer());

    command->set_render_pipeline(m_pipeline->get_interface());
    command->set_render_parameter(0, context.get_camera("main camera"));
    command->draw(0, 36);

    command->end();
}
} // namespace violet