#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"
#include "graphics/camera.hpp"

namespace violet::graphics
{
blinn_phong_material_pipeline_parameter::blinn_phong_material_pipeline_parameter()
    : pipeline_parameter(layout)
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

blinn_phong_pipeline::blinn_phong_pipeline()
{
    render_pipeline_desc desc;

    // Color pass.
    render_pass_desc& color_pass = desc.passes[0];
    color_pass.vertex_shader = "engine/shader/blinn_phong.vert";
    color_pass.pixel_shader = "engine/shader/blinn_phong.frag";

    color_pass.vertex_attributes[0] = {"POSITION", VERTEX_ATTRIBUTE_TYPE_FLOAT3}; // position
    color_pass.vertex_attributes[1] = {"NORMAL", VERTEX_ATTRIBUTE_TYPE_FLOAT3};   // normal
    color_pass.vertex_attribute_count = 2;

    color_pass.references[0] = {ATTACHMENT_REFERENCE_TYPE_COLOR, 0};
    color_pass.references[1] = {ATTACHMENT_REFERENCE_TYPE_DEPTH, 0};
    color_pass.references[2] = {ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0};
    color_pass.reference_count = 3;

    color_pass.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    color_pass.parameters[0] = object_pipeline_parameter::layout;
    color_pass.parameters[1] = blinn_phong_material_pipeline_parameter::layout;
    color_pass.parameters[2] = camera_pipeline_parameter::layout;
    color_pass.parameters[3] = light_pipeline_parameter::layout;

    color_pass.parameter_count = 4;

    //, "violet_blinn_phong_material", "violet_camera", "violet_light"};
    color_pass.samples = 4;

    desc.pass_count = 1;

    // Attachment.
    attachment_desc& render_target = desc.attachments[0];
    render_target.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = rhi::back_buffer_format();
    render_target.load_op = ATTACHMENT_LOAD_OP_LOAD;
    render_target.store_op = ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = RESOURCE_STATE_RENDER_TARGET;

    attachment_desc& depth_stencil = desc.attachments[1];
    depth_stencil.type = ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = ATTACHMENT_LOAD_OP_LOAD;
    depth_stencil.store_op = ATTACHMENT_STORE_OP_STORE;
    depth_stencil.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = RESOURCE_STATE_DEPTH_STENCIL;

    attachment_desc& render_target_resolve = desc.attachments[2];
    render_target_resolve.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    render_target_resolve.format = rhi::back_buffer_format();
    render_target_resolve.load_op = ATTACHMENT_LOAD_OP_CLEAR;
    render_target_resolve.store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target_resolve.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target_resolve.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target_resolve.samples = 1;
    render_target_resolve.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target_resolve.final_state = RESOURCE_STATE_PRESENT;

    desc.attachment_count = 3;

    m_interface = rhi::make_render_pipeline(desc);
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
} // namespace violet::graphics