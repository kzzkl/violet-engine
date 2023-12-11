#include "pbr_render.hpp"
#include "graphics/pipeline/skybox_pipeline.hpp"

namespace violet::sample
{
pbr_render_graph::pbr_render_graph(renderer* renderer) : render_graph(renderer)
{
    render_pass* main = add_render_pass("main");

    render_attachment* output_attachment = main->add_attachment("output");
    output_attachment->set_format(renderer->get_back_buffer()->get_format());
    output_attachment->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
    output_attachment->set_final_state(RHI_RESOURCE_STATE_PRESENT);
    output_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    output_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

    render_attachment* depth_stencil_attachment = main->add_attachment("depth stencil");
    depth_stencil_attachment->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
    depth_stencil_attachment->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
    depth_stencil_attachment->set_final_state(RHI_RESOURCE_STATE_DEPTH_STENCIL);
    depth_stencil_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    depth_stencil_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);
    depth_stencil_attachment->set_stencil_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
    depth_stencil_attachment->set_stencil_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);

    render_subpass* color_pass = main->add_subpass("color pass");
    color_pass->add_reference(
        output_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
        RHI_RESOURCE_STATE_RENDER_TARGET);
    color_pass->add_reference(
        depth_stencil_attachment,
        RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
        RHI_RESOURCE_STATE_DEPTH_STENCIL);

    color_pass->add_pipeline<skybox_pipeline>("skybox");

    compile();
}
} // namespace violet::sample