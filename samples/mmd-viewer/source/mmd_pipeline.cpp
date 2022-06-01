#include "mmd_pipeline.hpp"

namespace ash::sample::mmd
{
mmd_render_pipeline::mmd_render_pipeline()
{
    auto& graphics = system<graphics::graphics>();

    graphics::pipeline_parameter_layout_info mmd_material;
    mmd_material.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4,          1}, // diffuse
        {graphics::pipeline_parameter_type::FLOAT3,          1}, // specular
        {graphics::pipeline_parameter_type::FLOAT,           1}, // specular_strength
        {graphics::pipeline_parameter_type::UINT,            1}, // toon_mode
        {graphics::pipeline_parameter_type::UINT,            1}, // spa_mode
        {graphics::pipeline_parameter_type::SHADER_RESOURCE, 1}, // tex
        {graphics::pipeline_parameter_type::SHADER_RESOURCE, 1}, // toon
        {graphics::pipeline_parameter_type::SHADER_RESOURCE, 1}  // spa
    };
    graphics.make_pipeline_parameter_layout("mmd_material", mmd_material);

    // Color pass.
    graphics::pipeline_info color_pass_info = {};
    color_pass_info.vertex_shader = "resource/shader/color.vert";
    color_pass_info.pixel_shader = "resource/shader/color.frag";
    color_pass_info.vertex_attributes = {
        {"POSITION", graphics::vertex_attribute_type::FLOAT3}, // position
        {"NORMAL",   graphics::vertex_attribute_type::FLOAT3}, // normal
        {"UV",       graphics::vertex_attribute_type::FLOAT2}, // uv
    };
    color_pass_info.references = {
        {graphics::attachment_reference_type::COLOR, 0},
        {graphics::attachment_reference_type::DEPTH, 0},
        {graphics::attachment_reference_type::UNUSE, 0}
    };
    color_pass_info.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    color_pass_info.parameters = {"ash_object", "mmd_material", "ash_pass"};
    color_pass_info.samples = 4;

    // Edge pass.
    graphics::pipeline_info edge_pass_info = {};
    edge_pass_info.vertex_shader = "resource/shader/edge.vert";
    edge_pass_info.pixel_shader = "resource/shader/edge.frag";
    edge_pass_info.vertex_attributes = {
        {"POSITION", graphics::vertex_attribute_type::FLOAT3}, // position
        {"NORMAL",   graphics::vertex_attribute_type::FLOAT3}, // normal
    };
    edge_pass_info.references = {
        {graphics::attachment_reference_type::COLOR,   0},
        {graphics::attachment_reference_type::DEPTH,   0},
        {graphics::attachment_reference_type::RESOLVE, 0}
    };
    edge_pass_info.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    edge_pass_info.parameters = {"ash_object"};
    edge_pass_info.samples = 4;
    edge_pass_info.rasterizer.cull_mode = graphics::cull_mode::FRONT;
    edge_pass_info.depth_stencil.depth_functor = graphics::depth_functor::LESS;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.type = graphics::attachment_type::CAMERA_RENDER_TARGET;
    render_target.format = graphics.back_buffer_format();
    render_target.load_op = graphics::attachment_load_op::CLEAR;
    render_target.store_op = graphics::attachment_store_op::STORE;
    render_target.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    render_target.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::resource_state::RENDER_TARGET;
    render_target.final_state = graphics::resource_state::RENDER_TARGET;

    graphics::attachment_info depth_stencil = {};
    depth_stencil.type = graphics::attachment_type::CAMERA_DEPTH_STENCIL;
    depth_stencil.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::attachment_load_op::CLEAR;
    depth_stencil.store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    depth_stencil.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::resource_state::DEPTH_STENCIL;
    depth_stencil.final_state = graphics::resource_state::DEPTH_STENCIL;

    graphics::attachment_info back_buffer = {};
    back_buffer.type = graphics::attachment_type::CAMERA_RENDER_TARGET_RESOLVE;
    back_buffer.format = graphics.back_buffer_format();
    back_buffer.load_op = graphics::attachment_load_op::CLEAR;
    back_buffer.store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    back_buffer.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::resource_state::RENDER_TARGET;
    back_buffer.final_state = graphics::resource_state::PRESENT;

    graphics::render_pass_info mmd_pass_info;
    mmd_pass_info.attachments.push_back(render_target);
    mmd_pass_info.attachments.push_back(depth_stencil);
    mmd_pass_info.attachments.push_back(back_buffer);
    mmd_pass_info.subpasses.push_back(color_pass_info);
    mmd_pass_info.subpasses.push_back(edge_pass_info);

    m_interface = graphics.make_render_pass(mmd_pass_info);
}

void mmd_render_pipeline::render(
    const graphics::camera& camera,
    graphics::render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        camera.render_target(),
        camera.render_target_resolve(),
        camera.depth_stencil_buffer());

    graphics::scissor_extent rect = {};
    auto [width, height] = camera.render_target()->extent();
    rect.max_x = width;
    rect.max_y = height;
    command->scissor(&rect, 1);

    command->parameter(2, camera.parameter()->interface());

    // Color pass.
    for (auto& unit : units())
    {
        command->parameter(0, unit.parameters[0]->interface());
        command->parameter(1, unit.parameters[1]->interface());

        command->draw(
            unit.vertex_buffers.data(),
            unit.vertex_buffers.size(),
            unit.index_buffer,
            unit.index_start,
            unit.index_end,
            unit.vertex_base);
    }

    command->next(m_interface.get());

    // Edge pass.
    for (auto& unit : units())
    {
        command->parameter(0, unit.parameters[0]->interface());

        std::array<graphics::resource*, 2> vertex_buffers = {
            unit.vertex_buffers[0],
            unit.vertex_buffers[1]};
        command->draw(
            vertex_buffers.data(),
            vertex_buffers.size(),
            unit.index_buffer,
            unit.index_start,
            unit.index_end,
            unit.vertex_base);
    }

    command->end(m_interface.get());
}

mmd_skin_pipeline::mmd_skin_pipeline()
{
    auto& graphics = system<graphics::graphics>();

    graphics::pipeline_parameter_layout_info mmd_skin;
    mmd_skin.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4x4_ARRAY,   512}, // bone transform.
        {graphics::pipeline_parameter_type::SHADER_RESOURCE,  1  }, // input position.
        {graphics::pipeline_parameter_type::SHADER_RESOURCE,  1  }, // input normal.
        {graphics::pipeline_parameter_type::SHADER_RESOURCE,  1  }, // bone index.
        {graphics::pipeline_parameter_type::SHADER_RESOURCE,  1  }, // bone weight.
        {graphics::pipeline_parameter_type::UNORDERED_ACCESS, 1  }, // output position.
        {graphics::pipeline_parameter_type::UNORDERED_ACCESS, 1  }  // output normal.
    };
    graphics.make_pipeline_parameter_layout("mmd_skin", mmd_skin);

    graphics::compute_pipeline_info compute_pipeline_info = {};
    compute_pipeline_info.compute_shader = "resource/shader/skin.comp";
    compute_pipeline_info.parameters = {"mmd_skin"};

    m_interface = system<graphics::graphics>().make_compute_pipeline(compute_pipeline_info);
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
        command->compute_parameter(0, unit.parameter->interface());

        command->dispatch(unit.vertex_count / 256 + 256, 1, 1);
    }

    command->end(m_interface.get());
}
} // namespace ash::sample::mmd