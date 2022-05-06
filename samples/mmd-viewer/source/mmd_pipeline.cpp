#include "mmd_pipeline.hpp"

namespace ash::sample::mmd
{
mmd_pass::mmd_pass(graphics::graphics& graphics)
{
    initialize_interface(graphics);
    initialize_attachment_set(graphics);
}

void mmd_pass::render(const graphics::camera& camera, graphics::render_command_interface* command)
{
    static std::size_t counter = 0;
    command->begin(m_interface.get(), m_attachment_sets[counter].get());

    command->parameter(3, camera.parameter->parameter());
    for (auto& unit : units())
    {
        command->parameter(0, unit->parameters[0]->parameter());
        command->parameter(1, unit->parameters[1]->parameter());
        command->parameter(2, unit->parameters[2]->parameter());

        command->draw(
            unit->vertex_buffer,
            unit->index_buffer,
            unit->index_start,
            unit->index_end,
            unit->vertex_base);
    }

    command->end(m_interface.get());

    counter = (counter + 1) % m_attachment_sets.size();
}

void mmd_pass::initialize_interface(graphics::graphics& graphics)
{
    graphics::pass_parameter_layout_info mmd_material;
    mmd_material.parameters = {
        {graphics::pass_parameter_type::FLOAT4,  1}, // diffuse
        {graphics::pass_parameter_type::FLOAT3,  1}, // specular
        {graphics::pass_parameter_type::FLOAT,   1}, // specular_strength
        {graphics::pass_parameter_type::UINT,    1}, // toon_mode
        {graphics::pass_parameter_type::UINT,    1}, // spa_mode
        {graphics::pass_parameter_type::TEXTURE, 1}, // tex
        {graphics::pass_parameter_type::TEXTURE, 1}, // toon
        {graphics::pass_parameter_type::TEXTURE, 1}  // spa
    };
    graphics.make_render_parameter_layout("mmd_material", mmd_material);

    graphics::pass_parameter_layout_info mmd_skeleton;
    mmd_skeleton.parameters = {
        {graphics::pass_parameter_type::FLOAT4x4_ARRAY, 512}, // offset
    };
    graphics.make_render_parameter_layout("mmd_skeleton", mmd_skeleton);

    // Pass.
    graphics::pass_info color_pass_info = {};
    color_pass_info.vertex_shader = "resource/shader/glsl/vert.spv";
    color_pass_info.pixel_shader = "resource/shader/glsl/frag.spv";
    color_pass_info.vertex_layout.attributes = {
        graphics::vertex_attribute_type::FLOAT3, // position
        graphics::vertex_attribute_type::FLOAT3, // normal
        graphics::vertex_attribute_type::FLOAT2, // uv
        graphics::vertex_attribute_type::UINT4,  // bone
        graphics::vertex_attribute_type::FLOAT3, // bone weight
    };
    color_pass_info.references = {
        {graphics::attachment_reference_type::COLOR,   0},
        {graphics::attachment_reference_type::DEPTH,   0},
        {graphics::attachment_reference_type::RESOLVE, 0}
    };
    color_pass_info.primitive_topology = graphics::primitive_topology::TRIANGLE_LIST;
    color_pass_info.pass_layout_info
        .parameters = {"ash_object", "mmd_material", "mmd_skeleton", "ash_pass"};
    color_pass_info.samples = 4;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.format = graphics.back_buffers()[0]->format();
    render_target.load_op = graphics::attachment_load_op::CLEAR;
    render_target.store_op = graphics::attachment_store_op::STORE;
    render_target.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    render_target.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::resource_state::UNDEFINED;
    render_target.final_state = graphics::resource_state::PRESENT;

    graphics::attachment_info depth_stencil = {};
    depth_stencil.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::attachment_load_op::CLEAR;
    depth_stencil.store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    depth_stencil.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::resource_state::UNDEFINED;
    depth_stencil.final_state = graphics::resource_state::DEPTH_STENCIL;

    graphics::attachment_info back_buffer = {};
    back_buffer.format = graphics::resource_format::R8G8B8A8_UNORM;
    back_buffer.load_op = graphics::attachment_load_op::CLEAR;
    back_buffer.store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    back_buffer.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::resource_state::UNDEFINED;
    back_buffer.final_state = graphics::resource_state::PRESENT;

    graphics::technique_info mmd_technique_info;
    mmd_technique_info.attachments.push_back(render_target);
    mmd_technique_info.attachments.push_back(depth_stencil);
    mmd_technique_info.attachments.push_back(back_buffer);
    mmd_technique_info.subpasses.push_back(color_pass_info);

    m_interface = graphics.make_technique(mmd_technique_info);
}

void mmd_pass::initialize_attachment_set(graphics::graphics& graphics)
{
    auto back_buffers = graphics.back_buffers();

    graphics::render_target_desc render_target_desc = {};
    render_target_desc.width = 1300;
    render_target_desc.height = 800;
    render_target_desc.format = graphics::resource_format::R8G8B8A8_UNORM;
    render_target_desc.samples = 4;
    m_render_target = graphics.make_render_target(render_target_desc);

    graphics::depth_stencil_desc depth_stencil_desc = {};
    depth_stencil_desc.width = 1300;
    depth_stencil_desc.height = 800;
    depth_stencil_desc.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil_desc.samples = 4;
    m_depth_stencil = graphics.make_depth_stencil(depth_stencil_desc);

    for (std::size_t i = 0; i < back_buffers.size(); ++i)
    {
        graphics::attachment_set_info attachment_set_info;
        attachment_set_info.attachments.push_back(m_render_target.get());
        attachment_set_info.attachments.push_back(m_depth_stencil.get());
        attachment_set_info.attachments.push_back(back_buffers[i]);
        attachment_set_info.technique = m_interface.get();
        attachment_set_info.width = 1300;
        attachment_set_info.height = 800;
        m_attachment_sets.emplace_back(graphics.make_attachment_set(attachment_set_info));
    }
}
} // namespace ash::sample::mmd