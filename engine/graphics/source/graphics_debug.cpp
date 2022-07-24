#pragma once

#include "graphics/graphics_debug.hpp"
#include "core/link.hpp"
#include "core/relation.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/render_pipeline.hpp"
#include "graphics/rhi.hpp"
#include "scene/scene.hpp"

namespace ash::graphics
{
debug_pipeline::debug_pipeline()
{
    // Color pass.
    render_pass_info color_pass_info = {};
    color_pass_info.vertex_shader = "engine/shader/debug.vert";
    color_pass_info.pixel_shader = "engine/shader/debug.frag";
    color_pass_info.vertex_attributes = {
        {"POSITION", VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // position
        {"COLOR",    VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // color
    };
    color_pass_info.references = {
        {ATTACHMENT_REFERENCE_TYPE_COLOR,   0},
        {ATTACHMENT_REFERENCE_TYPE_DEPTH,   0},
        {ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0}
    };
    color_pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_LINE;
    color_pass_info.parameters = {"ash_camera"};
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

    render_pipeline_info pipeline_info;
    pipeline_info.attachments.push_back(render_target);
    pipeline_info.attachments.push_back(depth_stencil);
    pipeline_info.attachments.push_back(render_target_resolve);
    pipeline_info.passes.push_back(color_pass_info);

    m_interface = rhi::make_render_pipeline(pipeline_info);
}

void debug_pipeline::on_render(const render_scene& scene, render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        scene.render_target,
        scene.render_target_resolve,
        scene.depth_stencil_buffer);

    scissor_extent extent = {};
    auto [width, height] = scene.render_target->extent();
    extent.max_x = width;
    extent.max_y = height;
    command->scissor(&extent, 1);

    command->parameter(0, scene.camera_parameter);
    for (auto& item : scene.items)
    {
        if (item.index_start == item.index_end)
            continue;

        command->input_assembly_state(
            item.vertex_buffers,
            2,
            item.index_buffer,
            PRIMITIVE_TOPOLOGY_LINE_LIST);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }

    command->end(m_interface.get());
}

graphics_debug::graphics_debug(std::size_t frame_resource)
    : m_vertex_buffers(2),
      m_frame_resource(frame_resource)
{
}

void graphics_debug::initialize()
{
    m_pipeline = std::make_unique<debug_pipeline>();

    m_vertex_buffers[0] = rhi::make_vertex_buffer<math::float3>(
        nullptr,
        MAX_VERTEX_COUNT * m_frame_resource,
        VERTEX_BUFFER_FLAG_NONE,
        true);
    m_vertex_buffers[1] = rhi::make_vertex_buffer<math::float3>(
        nullptr,
        MAX_VERTEX_COUNT * m_frame_resource,
        VERTEX_BUFFER_FLAG_NONE,
        true);

    std::vector<std::uint32_t> index_data(MAX_VERTEX_COUNT);
    for (std::uint32_t i = 0; i < MAX_VERTEX_COUNT; ++i)
        index_data[i] = i;
    m_index_buffer = rhi::make_index_buffer(index_data.data(), index_data.size());

    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    m_entity = world.create("graphics debug");
    world.add<mesh_render, core::link>(m_entity);

    auto& v = world.component<mesh_render>(m_entity);
    v.render_groups = RENDER_GROUP_DEBUG;

    for (auto& vertex_buffer : m_vertex_buffers)
        v.vertex_buffers.push_back(vertex_buffer.get());
    v.index_buffer = m_index_buffer.get();

    v.submeshes.push_back(submesh{
        .index_start = 0,
        .index_end = 0,
        .vertex_base = 0,
    });
    v.materials.push_back(material{.pipeline = m_pipeline.get(), .parameters = {}});

    relation.link(m_entity, scene.root());
}

void graphics_debug::sync()
{
    auto& world = system<ecs::world>();
    auto& v = world.component<mesh_render>(m_entity);

    m_vertex_buffers[0]->upload(
        m_vertex_position.data(),
        sizeof(math::float3) * m_vertex_position.size(),
        sizeof(math::float3) * v.submeshes[0].vertex_base);
    m_vertex_buffers[1]->upload(
        m_vertex_color.data(),
        sizeof(math::float3) * m_vertex_color.size(),
        sizeof(math::float3) * v.submeshes[0].vertex_base);

    v.submeshes[0].index_start = 0;
    v.submeshes[0].index_end = m_vertex_position.size();
}

void graphics_debug::next_frame()
{
    m_vertex_position.clear();
    m_vertex_color.clear();

    auto& world = system<ecs::world>();
    auto& v = world.component<mesh_render>(m_entity);

    v.submeshes[0].vertex_base += MAX_VERTEX_COUNT;
    v.submeshes[0].vertex_base %= MAX_VERTEX_COUNT * m_frame_resource;
}

void graphics_debug::draw_line(
    const math::float3& start,
    const math::float3& end,
    const math::float3& color)
{
    m_vertex_position.push_back(start);
    m_vertex_position.push_back(end);
    m_vertex_color.push_back(color);
    m_vertex_color.push_back(color);
}

void graphics_debug::draw_aabb(
    const math::float3& min,
    const math::float3& max,
    const math::float3& color)
{
    math::float3 t1 = {min[0], max[1], max[2]};
    math::float3 t2 = {max[0], max[1], max[2]};
    math::float3 t3 = {max[0], max[1], min[2]};
    math::float3 t4 = {min[0], max[1], min[2]};

    math::float3 b1 = {min[0], min[1], max[2]};
    math::float3 b2 = {max[0], min[1], max[2]};
    math::float3 b3 = {max[0], min[1], min[2]};
    math::float3 b4 = {min[0], min[1], min[2]};

    m_vertex_position.insert(m_vertex_position.end(), {t1, b1, t2, b2, t3, b3, t4, b4,
                                                       t1, t2, b1, b2, b3, b4, t3, t4,
                                                       t1, t4, t2, t3, b1, b4, b2, b3});
    m_vertex_color.insert(m_vertex_color.end(), 24, color);
}
} // namespace ash::graphics