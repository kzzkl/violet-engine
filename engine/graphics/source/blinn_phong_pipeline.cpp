#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/rhi.hpp"

namespace ash::graphics
{
blinn_phong_material_pipeline_parameter::blinn_phong_material_pipeline_parameter()
    : pipeline_parameter("ash_blinn_phong_material")
{
}

void blinn_phong_material_pipeline_parameter::diffuse(const math::float3& diffuse)
{
    field<constant_data>(0).diffuse = diffuse;
}

void blinn_phong_material_pipeline_parameter::fresnel(const math::float3& fresnel)
{
    field<constant_data>(0).fresnel = fresnel;
}

void blinn_phong_material_pipeline_parameter::roughness(float roughness)
{
    field<constant_data>(0).roughness = roughness;
}

std::vector<pipeline_parameter_pair> blinn_phong_material_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(constant_data)}
    };
}

blinn_phong_pipeline::blinn_phong_pipeline()
{
    // Color pass.
    render_pass_info color_pass_info = {};
    color_pass_info.vertex_shader = "engine/shader/blinn_phong.vert";
    color_pass_info.pixel_shader = "engine/shader/blinn_phong.frag";
    color_pass_info.vertex_attributes = {
        {"POSITION", VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // position
        {"NORMAL",   VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // normal
    };
    color_pass_info.references = {
        {ATTACHMENT_REFERENCE_TYPE_COLOR,   0},
        {ATTACHMENT_REFERENCE_TYPE_DEPTH,   0},
        {ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0}
    };
    color_pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    color_pass_info
        .parameters = {"ash_object", "ash_blinn_phong_material", "ash_camera", "ash_light"};
    color_pass_info.samples = 4;

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

    render_pipeline_info blinn_phong_pipeline_info;
    blinn_phong_pipeline_info.attachments.push_back(render_target);
    blinn_phong_pipeline_info.attachments.push_back(depth_stencil);
    blinn_phong_pipeline_info.attachments.push_back(render_target_resolve);
    blinn_phong_pipeline_info.passes.push_back(color_pass_info);

    m_interface = rhi::make_render_pipeline(blinn_phong_pipeline_info);
}

void blinn_phong_pipeline::render(const render_context& context, render_command_interface* command)
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

    command->parameter(2, context.camera_parameter);
    command->parameter(3, context.light_parameter);
    for (auto& item : context.items)
    {
        command->parameter(0, item.object_parameter);
        command->parameter(1, item.material_parameter); // material

        command->input_assembly_state(item.vertex_buffers, 2, item.index_buffer);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }

    command->end(m_interface.get());
}
} // namespace ash::graphics