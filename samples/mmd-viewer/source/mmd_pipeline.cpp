#include "mmd_pipeline.hpp"
#include "common/assert.hpp"
#include "graphics/camera.hpp"
#include "graphics/rhi.hpp"

namespace violet::sample::mmd
{
mmd_material_parameter::mmd_material_parameter() : graphics::pipeline_parameter(layout)
{
}

void mmd_material_parameter::diffuse(const math::float4& diffuse)
{
    field<constant_data>(0).diffuse = diffuse;
}

void mmd_material_parameter::specular(const math::float3& specular)
{
    field<constant_data>(0).specular = specular;
}

void mmd_material_parameter::specular_strength(float specular_strength)
{
    field<constant_data>(0).specular_strength = specular_strength;
}

void mmd_material_parameter::edge_color(const math::float4& edge_color)
{
    field<constant_data>(0).edge_color = edge_color;
}

void mmd_material_parameter::edge_size(float edge_size)
{
    field<constant_data>(0).edge_size = edge_size;
}

void mmd_material_parameter::toon_mode(std::uint32_t toon_mode)
{
    field<constant_data>(0).toon_mode = toon_mode;
}

void mmd_material_parameter::spa_mode(std::uint32_t spa_mode)
{
    field<constant_data>(0).spa_mode = spa_mode;
}

void mmd_material_parameter::tex(graphics::resource_interface* tex)
{
    interface()->set(1, tex);
}

void mmd_material_parameter::toon(graphics::resource_interface* toon)
{
    interface()->set(2, toon);
}

void mmd_material_parameter::spa(graphics::resource_interface* spa)
{
    interface()->set(3, spa);
}

mmd_render_pipeline::mmd_render_pipeline()
{
    graphics::render_pipeline_desc desc;

    // Color pass.
    graphics::render_pass_desc& color_pass = desc.passes[0];
    color_pass.vertex_shader = "mmd-viewer/shader/color.vert";
    color_pass.pixel_shader = "mmd-viewer/shader/color.frag";

    color_pass.vertex_attributes[0] = {
        "POSITION",
        graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}; // position
    color_pass.vertex_attributes[1] = {"NORMAL", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}; // normal
    color_pass.vertex_attributes[2] = {"UV", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT2};     // uv
    color_pass.vertex_attribute_count = 3;

    color_pass.references[0] = {graphics::ATTACHMENT_REFERENCE_TYPE_COLOR, 0};
    color_pass.references[1] = {graphics::ATTACHMENT_REFERENCE_TYPE_DEPTH, 0};
    color_pass.references[2] = {graphics::ATTACHMENT_REFERENCE_TYPE_UNUSE, 0};
    color_pass.reference_count = 3;

    color_pass.parameters[0] = graphics::object_pipeline_parameter::layout;
    color_pass.parameters[1] = mmd_material_parameter::layout;
    color_pass.parameters[2] = graphics::camera_pipeline_parameter::layout;
    color_pass.parameters[3] = graphics::light_pipeline_parameter::layout;
    color_pass.parameter_count = 4;

    color_pass.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    color_pass.samples = 4;

    // Edge pass.
    graphics::render_pass_desc& edge_pass = desc.passes[1];
    edge_pass.vertex_shader = "mmd-viewer/shader/edge.vert";
    edge_pass.pixel_shader = "mmd-viewer/shader/edge.frag";

    edge_pass.vertex_attributes[0] = {
        "POSITION",
        graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3};                                         // position
    edge_pass.vertex_attributes[1] = {"NORMAL", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT3}; // normal
    edge_pass.vertex_attributes[2] = {"EDGE", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT};    // edge
    edge_pass.vertex_attribute_count = 3;

    edge_pass.references[0] = {graphics::ATTACHMENT_REFERENCE_TYPE_COLOR, 0};
    edge_pass.references[1] = {graphics::ATTACHMENT_REFERENCE_TYPE_DEPTH, 0};
    edge_pass.references[2] = {graphics::ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0};
    edge_pass.reference_count = 3;

    edge_pass.parameters[0] = graphics::object_pipeline_parameter::layout;
    edge_pass.parameters[1] = mmd_material_parameter::layout;
    edge_pass.parameters[2] = graphics::camera_pipeline_parameter::layout;
    edge_pass.parameter_count = 3;

    edge_pass.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    edge_pass.samples = 4;
    edge_pass.rasterizer.cull_mode = graphics::CULL_MODE_FRONT;
    edge_pass.depth_stencil.depth_functor = graphics::DEPTH_STENCIL_FUNCTOR_LESS;

    desc.pass_count = 2;

    // Attachment.
    graphics::attachment_desc& render_target = desc.attachments[0];
    render_target.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = graphics::rhi::back_buffer_format();
    render_target.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    render_target.store_op = graphics::ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = graphics::RESOURCE_STATE_RENDER_TARGET;

    graphics::attachment_desc& depth_stencil = desc.attachments[1];
    depth_stencil.type = graphics::ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    depth_stencil.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;

    graphics::attachment_desc& back_buffer = desc.attachments[2];
    back_buffer.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    back_buffer.format = graphics::rhi::back_buffer_format();
    back_buffer.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    back_buffer.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    back_buffer.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    back_buffer.final_state = graphics::RESOURCE_STATE_PRESENT;

    desc.attachment_count = 3;

    m_interface = graphics::rhi::make_render_pipeline(desc);
}

void mmd_render_pipeline::render(
    const graphics::render_context& context,
    graphics::render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        context.render_target,
        context.render_target_resolve,
        context.depth_stencil_buffer);

    graphics::scissor_extent rect = {};
    auto [width, height] = context.render_target->extent();
    rect.max_x = width;
    rect.max_y = height;
    command->scissor(&rect, 1);

    // Color pass.
    command->parameter(2, context.camera_parameter);
    command->parameter(3, context.light_parameter);
    for (auto& item : context.items)
    {
        command->parameter(0, item.object_parameter);
        command->parameter(1, item.material_parameter);

        graphics::resource_interface* vertex_buffers[] = {
            item.vertex_buffers[0],
            item.vertex_buffers[1],
            item.vertex_buffers[2]};
        command->input_assembly_state(vertex_buffers, 3, item.index_buffer);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }

    command->next_pass(m_interface.get());

    // Edge pass.
    command->parameter(2, context.camera_parameter);
    for (auto& item : context.items)
    {
        command->parameter(0, item.object_parameter);
        command->parameter(1, item.material_parameter);

        graphics::resource_interface* vertex_buffers[] = {
            item.vertex_buffers[0],
            item.vertex_buffers[1],
            item.vertex_buffers[3]};
        command->input_assembly_state(vertex_buffers, 3, item.index_buffer);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }

    command->end(m_interface.get());
}

skinning_pipeline_parameter::skinning_pipeline_parameter() : graphics::pipeline_parameter(layout)
{
}

void skinning_pipeline_parameter::bone_transform(const std::vector<math::float4x4>& bone_transform)
{
    auto& constant = field<constant_data>(0);
    VIOLET_ASSERT(bone_transform.size() <= constant.bone_transform.size());

    for (std::size_t i = 0; i < bone_transform.size(); ++i)
    {
        math::float4x4_simd m = math::simd::load(bone_transform[i]);
        math::float4_simd q = math::quaternion_simd::rotation_matrix(m);

        math::simd::store(math::matrix_simd::transpose(m), constant.bone_transform[i]);
        math::simd::store(q, constant.bone_quaternion[i]);
    }
}

void skinning_pipeline_parameter::input_position(graphics::resource_interface* position)
{
    interface()->set(1, position);
}

void skinning_pipeline_parameter::input_normal(graphics::resource_interface* normal)
{
    interface()->set(2, normal);
}

void skinning_pipeline_parameter::input_uv(graphics::resource_interface* uv)
{
    interface()->set(3, uv);
}

void skinning_pipeline_parameter::skin(graphics::resource_interface* skin)
{
    interface()->set(4, skin);
}

void skinning_pipeline_parameter::bdef_bone(graphics::resource_interface* bdef_bone)
{
    interface()->set(5, bdef_bone);
}

void skinning_pipeline_parameter::sdef_bone(graphics::resource_interface* sdef_bone)
{
    interface()->set(6, sdef_bone);
}

void skinning_pipeline_parameter::vertex_morph(graphics::resource_interface* vertex_morph)
{
    interface()->set(7, vertex_morph);
}

void skinning_pipeline_parameter::uv_morph(graphics::resource_interface* uv_morph)
{
    interface()->set(8, uv_morph);
}

void skinning_pipeline_parameter::output_position(graphics::resource_interface* position)
{
    interface()->set(9, position);
}

void skinning_pipeline_parameter::output_normal(graphics::resource_interface* normal)
{
    interface()->set(10, normal);
}

void skinning_pipeline_parameter::output_uv(graphics::resource_interface* uv)
{
    interface()->set(11, uv);
}

mmd_skinning_pipeline::mmd_skinning_pipeline()
{
    graphics::compute_pipeline_desc compute_pipeline = {};
    compute_pipeline.compute_shader = "mmd-viewer/shader/skin.comp";

    compute_pipeline.parameters[0] = skinning_pipeline_parameter::layout;
    compute_pipeline.parameter_count = 1;

    m_interface = graphics::rhi::make_compute_pipeline(compute_pipeline);
}

void mmd_skinning_pipeline::on_skinning(
    const std::vector<graphics::skinning_item>& items,
    graphics::render_command_interface* command)
{
    command->begin(m_interface.get());

    for (auto& item : items)
    {
        command->compute_parameter(0, item.parameter);
        command->dispatch(item.vertex_count / 256 + 256, 1, 1);
    }

    command->end(m_interface.get());
}
} // namespace violet::sample::mmd