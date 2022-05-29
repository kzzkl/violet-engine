#pragma once

#include "graphics/graphics_debug.hpp"
#include "core/link.hpp"
#include "core/relation.hpp"
#include "graphics/graphics.hpp"
#include "graphics/render_pipeline.hpp"
#include "graphics/visual.hpp"
#include "scene/scene.hpp"

namespace ash::graphics
{
debug_pipeline::debug_pipeline(graphics& graphics)
{
    // Color pass.
    pipeline_info color_pass_info = {};
    color_pass_info.vertex_shader = "engine/shader/debug.vert";
    color_pass_info.pixel_shader = "engine/shader/debug.frag";
    color_pass_info.vertex_attributes = {
        {"POSITION", vertex_attribute_type::FLOAT3}, // position
        {"COLOR",    vertex_attribute_type::FLOAT3}, // color
    };
    color_pass_info.references = {
        {attachment_reference_type::COLOR,   0},
        {attachment_reference_type::DEPTH,   0},
        {attachment_reference_type::RESOLVE, 0}
    };
    color_pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_LINE;
    color_pass_info.parameters = {"ash_pass"};
    color_pass_info.samples = 4;

    // Attachment.
    attachment_info render_target = {};
    render_target.type = attachment_type::CAMERA_RENDER_TARGET;
    render_target.format = graphics.back_buffer_format();
    render_target.load_op = attachment_load_op::LOAD;
    render_target.store_op = attachment_store_op::STORE;
    render_target.stencil_load_op = attachment_load_op::DONT_CARE;
    render_target.stencil_store_op = attachment_store_op::DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = resource_state::RENDER_TARGET;
    render_target.final_state = resource_state::RENDER_TARGET;

    attachment_info depth_stencil = {};
    depth_stencil.type = attachment_type::CAMERA_DEPTH_STENCIL;
    depth_stencil.format = resource_format::D24_UNORM_S8_UINT;
    depth_stencil.load_op = attachment_load_op::LOAD;
    depth_stencil.store_op = attachment_store_op::STORE;
    depth_stencil.stencil_load_op = attachment_load_op::DONT_CARE;
    depth_stencil.stencil_store_op = attachment_store_op::DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = resource_state::DEPTH_STENCIL;
    depth_stencil.final_state = resource_state::DEPTH_STENCIL;

    attachment_info render_target_resolve = {};
    render_target_resolve.type = attachment_type::CAMERA_RENDER_TARGET_RESOLVE;
    render_target_resolve.format = graphics.back_buffer_format();
    render_target_resolve.load_op = attachment_load_op::CLEAR;
    render_target_resolve.store_op = attachment_store_op::DONT_CARE;
    render_target_resolve.stencil_load_op = attachment_load_op::DONT_CARE;
    render_target_resolve.stencil_store_op = attachment_store_op::DONT_CARE;
    render_target_resolve.samples = 1;
    render_target_resolve.initial_state = resource_state::RENDER_TARGET;
    render_target_resolve.final_state = resource_state::PRESENT;

    render_pass_info pass_info;
    pass_info.attachments.push_back(render_target);
    pass_info.attachments.push_back(depth_stencil);
    pass_info.attachments.push_back(render_target_resolve);
    pass_info.subpasses.push_back(color_pass_info);

    m_interface = graphics.make_render_pass(pass_info);
}

void debug_pipeline::render(const camera& camera, render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        camera.render_target,
        camera.render_target_resolve,
        camera.depth_stencil_buffer);

    scissor_extent extent = {};
    auto [width, height] = camera.render_target->extent();
    extent.max_x = width;
    extent.max_y = height;
    command->scissor(&extent, 1);

    command->parameter(0, camera.parameter->parameter());
    for (auto& unit : units())
    {
        if (unit.index_start == unit.index_end)
            continue;

        command->draw(
            unit.vertex_buffers.data(),
            unit.vertex_buffers.size(),
            unit.index_buffer,
            unit.index_start,
            unit.index_end,
            unit.vertex_base,
            PRIMITIVE_TOPOLOGY_LINE_LIST);
    }

    command->end(m_interface.get());
}

graphics_debug::graphics_debug(std::size_t frame_resource)
    : m_vertex_buffers(2),
      m_frame_resource(frame_resource)
{
    auto& g = system<graphics>();
}

void graphics_debug::initialize(graphics& graphics)
{
    m_pipeline = std::make_unique<debug_pipeline>(graphics);

    m_vertex_buffers[0] = graphics.make_vertex_buffer<math::float3>(
        nullptr,
        MAX_VERTEX_COUNT * m_frame_resource,
        VERTEX_BUFFER_FLAG_NONE,
        true);
    m_vertex_buffers[1] = graphics.make_vertex_buffer<math::float3>(
        nullptr,
        MAX_VERTEX_COUNT * m_frame_resource,
        VERTEX_BUFFER_FLAG_NONE,
        true);

    std::vector<std::uint32_t> index_data(MAX_VERTEX_COUNT);
    for (std::uint32_t i = 0; i < MAX_VERTEX_COUNT; ++i)
        index_data[i] = i;
    m_index_buffer = graphics.make_index_buffer(index_data.data(), index_data.size());

    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    m_entity = world.create("graphics debug");
    world.add<visual, core::link>(m_entity);

    auto& v = world.component<visual>(m_entity);
    v.groups = VISUAL_GROUP_DEBUG;

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
    auto& v = world.component<visual>(m_entity);

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
    auto& v = world.component<visual>(m_entity);

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
} // namespace ash::graphics