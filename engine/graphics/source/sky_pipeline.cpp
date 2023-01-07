#include "graphics/sky_pipeline.hpp"
#include "graphics/rhi.hpp"

namespace violet::graphics
{
sky_pipeline_parameter::sky_pipeline_parameter() : pipeline_parameter("violet_sky")
{
}

void sky_pipeline_parameter::texture(resource_interface* texture)
{
    interface()->set(0, texture);
}

std::vector<pipeline_parameter_pair> sky_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1}
    };
}

sky_pipeline::sky_pipeline()
{
    render_pass_info pass_info = {};
    pass_info.vertex_shader = "engine/shader/sky.vert";
    pass_info.pixel_shader = "engine/shader/sky.frag";
    pass_info.references = {
        {ATTACHMENT_REFERENCE_TYPE_COLOR,   0},
        {ATTACHMENT_REFERENCE_TYPE_DEPTH,   0},
        {ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0}
    };
    pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pass_info.parameters = {"violet_camera", "violet_sky"};
    pass_info.samples = 4;
    pass_info.rasterizer.cull_mode = CULL_MODE_NONE;
    pass_info.depth_stencil.depth_functor = DEPTH_FUNCTOR_LESS_EQUAL;

    // Attachment.
    attachment_info render_target = {};
    render_target.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = rhi::back_buffer_format();
    render_target.load_op = ATTACHMENT_LOAD_OP_LOAD;
    render_target.store_op = ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = RESOURCE_STATE_RENDER_TARGET;

    attachment_info depth_stencil = {};
    depth_stencil.type = ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = ATTACHMENT_LOAD_OP_LOAD;
    depth_stencil.store_op = ATTACHMENT_STORE_OP_STORE;
    depth_stencil.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = RESOURCE_STATE_DEPTH_STENCIL;

    attachment_info render_target_resolve = {};
    render_target_resolve.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    render_target_resolve.format = rhi::back_buffer_format();
    render_target_resolve.load_op = ATTACHMENT_LOAD_OP_CLEAR;
    render_target_resolve.store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target_resolve.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target_resolve.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target_resolve.samples = 1;
    render_target_resolve.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target_resolve.final_state = RESOURCE_STATE_PRESENT;

    render_pipeline_info pipeline_info;
    pipeline_info.attachments.push_back(render_target);
    pipeline_info.attachments.push_back(depth_stencil);
    pipeline_info.attachments.push_back(render_target_resolve);
    pipeline_info.passes.push_back(pass_info);

    m_interface = rhi::make_render_pipeline(pipeline_info);
}

void sky_pipeline::render(const render_context& context, render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        context.render_target,
        context.render_target_resolve,
        context.depth_stencil_buffer);

    scissor_extent extent = {};
    auto [width, height] = context.render_target->extent();
    extent.max_x = width;
    extent.max_y = height;
    command->scissor(&extent, 1);

    command->parameter(0, context.camera_parameter);
    command->parameter(1, context.sky_parameter);

    command->input_assembly_state(nullptr, 0, nullptr, PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    command->draw(0, 36);

    command->end(m_interface.get());
}
} // namespace violet::graphics