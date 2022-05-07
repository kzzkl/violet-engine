#pragma once

#include "graphics_debug.hpp"
#include "graphics.hpp"
#include "link.hpp"
#include "render_pipeline.hpp"
#include "visual.hpp"

namespace ash::graphics
{
debug_pass::debug_pass(render_pass_interface* interface)
{
}

void debug_pass::render(const camera& camera, render_command_interface* command)
{
    /*command->begin(interface(), nullptr);
    command->parameter(0, nullptr);

    for (auto& unit : units())
        command->draw(unit->vertex_buffer, unit->index_buffer, 0, unit->index_end, 0);*/
}

graphics_debug::graphics_debug(std::size_t frame_resource, graphics& graphics, ecs::world& world)
    : m_world(world),
      m_index(0),
      m_vertex_buffer(frame_resource)
{
    /*static constexpr std::size_t MAX_VERTEX_COUNT = 4096 * 16;

    m_pipeline = graphics.make_render_pipeline<debug_pass>("debug");
    for (auto& buffer : m_vertex_buffer)
        buffer = graphics.make_vertex_buffer<vertex>(nullptr, MAX_VERTEX_COUNT, true);

    std::vector<std::uint32_t> index_data(MAX_VERTEX_COUNT * 2);
    for (std::uint32_t i = 0; i < MAX_VERTEX_COUNT * 2; ++i)
        index_data[i] = i;
    m_index_buffer = graphics.make_index_buffer(index_data.data(), index_data.size());*/
}

void graphics_debug::initialize()
{
    /*m_entity = m_world.create("graphics debug");
    m_world.add<visual, link>(m_entity);

    auto& v = m_world.component<visual>(m_entity);
    v.mask = visual::mask_type::DEBUG;
    v.submesh.resize(1);
    v.submesh[0].pipeline = m_pipeline.get();*/
}

void graphics_debug::sync()
{
    /*m_vertex_buffer[m_index]->upload(m_vertics.data(), sizeof(vertex) * m_vertics.size(), 0);

    auto& v = m_world.component<visual>(m_entity);
    v.submesh[0].vertex_buffer = m_vertex_buffer[m_index].get();
    v.submesh[0].index_buffer = m_index_buffer.get();
    v.submesh[0].index_end = m_vertics.size() * 2;*/
}

void graphics_debug::begin_frame()
{
}

void graphics_debug::end_frame()
{
    m_vertics.clear();
    m_index = (m_index + 1) % m_vertex_buffer.size();
}

void graphics_debug::draw_line(
    const math::float3& start,
    const math::float3& end,
    const math::float3& color)
{
    m_vertics.push_back(vertex{start, color});
    m_vertics.push_back(vertex{end, color});
}
} // namespace ash::graphics