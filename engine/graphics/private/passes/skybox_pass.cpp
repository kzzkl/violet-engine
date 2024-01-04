#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
skybox_pass::skybox_pass(renderer* renderer, setup_context& context)
    : render_pass(renderer, context)
{
    m_render_target = context.write("back buffer");
    m_depth_buffer = context.write("depth buffer");

    render_attachment* render_target_attachment = add_attachment(m_render_target);
    render_target_attachment->set_initial_layout(RHI_IMAGE_LAYOUT_RENDER_TARGET);
    render_target_attachment->set_final_layout(RHI_IMAGE_LAYOUT_PRESENT);
    render_target_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_LOAD);
    render_target_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

    render_attachment* depth_buffer_attachment = add_attachment(m_depth_buffer);
    depth_buffer_attachment->set_initial_layout(RHI_IMAGE_LAYOUT_DEPTH_STENCIL);
    depth_buffer_attachment->set_final_layout(RHI_IMAGE_LAYOUT_DEPTH_STENCIL);
    depth_buffer_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_LOAD);
    depth_buffer_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);
    depth_buffer_attachment->set_stencil_load_op(RHI_ATTACHMENT_LOAD_OP_LOAD);
    depth_buffer_attachment->set_stencil_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);

    render_subpass* subpass = add_subpass();
    subpass->add_reference(
        render_target_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
        RHI_IMAGE_LAYOUT_RENDER_TARGET);
    subpass->add_reference(
        depth_buffer_attachment,
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

    command->begin(get_interface(), get_framebuffer({m_render_target, m_depth_buffer}));

    command->set_render_pipeline(m_pipeline->get_interface());
    command->set_render_parameter(0, context.get_camera("main camera"));
    command->draw(0, 36);

    command->end();
}
} // namespace violet