#include "mmd_pipeline.hpp"
#include "assert.hpp"
#include "graphics/rhi.hpp"

namespace ash::sample::mmd
{
material_pipeline_parameter::material_pipeline_parameter()
    : graphics::pipeline_parameter("mmd_material")
{
}

void material_pipeline_parameter::diffuse(const math::float4& diffuse)
{
    field<constant_data>(0).diffuse = diffuse;
}

void material_pipeline_parameter::specular(const math::float3& specular)
{
    field<constant_data>(0).specular = specular;
}

void material_pipeline_parameter::specular_strength(float specular_strength)
{
    field<constant_data>(0).specular_strength = specular_strength;
}

void material_pipeline_parameter::toon_mode(std::uint32_t toon_mode)
{
    field<constant_data>(0).toon_mode = toon_mode;
}

void material_pipeline_parameter::spa_mode(std::uint32_t spa_mode)
{
    field<constant_data>(0).spa_mode = spa_mode;
}

void material_pipeline_parameter::tex(graphics::resource_interface* tex)
{
    interface()->set(1, tex);
}

void material_pipeline_parameter::toon(graphics::resource_interface* toon)
{
    interface()->set(2, toon);
}

void material_pipeline_parameter::spa(graphics::resource_interface* spa)
{
    interface()->set(3, spa);
}

std::vector<graphics::pipeline_parameter_pair> material_pipeline_parameter::layout()
{
    return {
        {graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER,
         sizeof(material_pipeline_parameter::constant_data)  }, // constant
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1}, // tex
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1}, // toon
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1}  // spa
    };
}

mmd_render_pipeline::mmd_render_pipeline()
{
    // Color pass.
    graphics::render_pass_info color_pass_info = {};
    color_pass_info.vertex_shader = "resource/shader/color.vert";
    color_pass_info.pixel_shader = "resource/shader/color.frag";
    color_pass_info.vertex_attributes = {
        {"POSITION", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // position
        {"NORMAL",   graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // normal
        {"UV",       graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT2}, // uv
    };
    color_pass_info.references = {
        {graphics::ATTACHMENT_REFERENCE_TYPE_COLOR, 0},
        {graphics::ATTACHMENT_REFERENCE_TYPE_DEPTH, 0},
        {graphics::ATTACHMENT_REFERENCE_TYPE_UNUSE, 0}
    };
    color_pass_info.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    color_pass_info.parameters = {"ash_object", "mmd_material", "ash_camera"};
    color_pass_info.samples = 4;

    // Edge pass.
    graphics::render_pass_info edge_pass_info = {};
    edge_pass_info.vertex_shader = "resource/shader/edge.vert";
    edge_pass_info.pixel_shader = "resource/shader/edge.frag";
    edge_pass_info.vertex_attributes = {
        {"POSITION", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // position
        {"NORMAL",   graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // normal
    };
    edge_pass_info.references = {
        {graphics::ATTACHMENT_REFERENCE_TYPE_COLOR,   0},
        {graphics::ATTACHMENT_REFERENCE_TYPE_DEPTH,   0},
        {graphics::ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0}
    };
    edge_pass_info.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    edge_pass_info.parameters = {"ash_object", "ash_camera"};
    edge_pass_info.samples = 4;
    edge_pass_info.rasterizer.cull_mode = graphics::CULL_MODE_FRONT;
    edge_pass_info.depth_stencil.depth_functor = graphics::DEPTH_FUNCTOR_LESS;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = graphics::rhi::back_buffer_format();
    render_target.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    render_target.store_op = graphics::ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = graphics::RESOURCE_STATE_RENDER_TARGET;

    graphics::attachment_info depth_stencil = {};
    depth_stencil.type = graphics::ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    depth_stencil.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;

    graphics::attachment_info back_buffer = {};
    back_buffer.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    back_buffer.format = graphics::rhi::back_buffer_format();
    back_buffer.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    back_buffer.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    back_buffer.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    back_buffer.final_state = graphics::RESOURCE_STATE_PRESENT;

    graphics::render_pipeline_info mmd_pipeline_info;
    mmd_pipeline_info.attachments.push_back(render_target);
    mmd_pipeline_info.attachments.push_back(depth_stencil);
    mmd_pipeline_info.attachments.push_back(back_buffer);
    mmd_pipeline_info.passes.push_back(color_pass_info);
    mmd_pipeline_info.passes.push_back(edge_pass_info);

    m_interface = graphics::rhi::make_render_pipeline(mmd_pipeline_info);
}

void mmd_render_pipeline::render(
    const graphics::render_scene& scene,
    graphics::render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        scene.render_target,
        scene.render_target_resolve,
        scene.depth_stencil_buffer);

    graphics::scissor_extent rect = {};
    auto [width, height] = scene.render_target->extent();
    rect.max_x = width;
    rect.max_y = height;
    command->scissor(&rect, 1);

    // Color pass.
    command->parameter(2, scene.camera_parameter);
    for (auto& unit : scene.units)
    {
        command->parameter(0, unit.parameters[0]);
        command->parameter(1, unit.parameters[1]);

        command->input_assembly_state(unit.vertex_buffers, 3, unit.index_buffer);
        command->draw_indexed(unit.index_start, unit.index_end, unit.vertex_base);
    }

    command->next_pass(m_interface.get());

    // Edge pass.
    command->parameter(1, scene.camera_parameter);
    for (auto& unit : scene.units)
    {
        command->parameter(0, unit.parameters[0]);

        command->input_assembly_state(unit.vertex_buffers, 2, unit.index_buffer);
        command->draw_indexed(unit.index_start, unit.index_end, unit.vertex_base);
    }

    command->end(m_interface.get());
}

skin_pipeline_parameter::skin_pipeline_parameter() : graphics::pipeline_parameter("mmd_skin")
{
}

void skin_pipeline_parameter::bone_transform(const std::vector<math::float4x4>& bone_transform)
{
    auto& constant = field<constant_data>(0);
    ASH_ASSERT(bone_transform.size() <= constant.bone_transform.size());

    for (std::size_t i = 0; i < bone_transform.size(); ++i)
        constant.bone_transform[i] = math::matrix::transpose(bone_transform[i]);
}

void skin_pipeline_parameter::input_position(graphics::resource_interface* position)
{
    interface()->set(1, position);
}

void skin_pipeline_parameter::input_normal(graphics::resource_interface* normal)
{
    interface()->set(2, normal);
}

void skin_pipeline_parameter::input_bone_index(graphics::resource_interface* bone_index)
{
    interface()->set(3, bone_index);
}

void skin_pipeline_parameter::input_bone_weight(graphics::resource_interface* bone_weight)
{
    interface()->set(4, bone_weight);
}

void skin_pipeline_parameter::output_position(graphics::resource_interface* position)
{
    interface()->set(5, position);
}

void skin_pipeline_parameter::output_normal(graphics::resource_interface* normal)
{
    interface()->set(6, normal);
}

std::vector<graphics::pipeline_parameter_pair> skin_pipeline_parameter::layout()
{
    return {
        {graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER,
         sizeof(constant_data)                                }, // bone transform.
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE,  1}, // input position.
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE,  1}, // input normal.
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE,  1}, // bone index.
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE,  1}, // bone weight.
        {graphics::PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS, 1}, // output position.
        {graphics::PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS, 1}  // output normal.
    };
}

mmd_skin_pipeline::mmd_skin_pipeline()
{
    graphics::compute_pipeline_info compute_pipeline_info = {};
    compute_pipeline_info.compute_shader = "resource/shader/skin.comp";
    compute_pipeline_info.parameters = {"mmd_skin"};

    m_interface = graphics::rhi::make_compute_pipeline(compute_pipeline_info);
}

void mmd_skin_pipeline::skin(graphics::render_command_interface* command)
{
    command->begin(m_interface.get());

    for (auto& unit : units())
    {
        unit.parameter->set(1, unit.input_vertex_buffers[0]);
        unit.parameter->set(2, unit.input_vertex_buffers[1]);
        unit.parameter->set(3, unit.input_vertex_buffers[2]);
        unit.parameter->set(4, unit.input_vertex_buffers[3]);
        unit.parameter->set(5, unit.skinned_vertex_buffers[0]);
        unit.parameter->set(6, unit.skinned_vertex_buffers[1]);
        command->compute_parameter(0, unit.parameter);

        command->dispatch(unit.vertex_count / 256 + 256, 1, 1);
    }

    command->end(m_interface.get());
}
} // namespace ash::sample::mmd