#include "physics_debug.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/materials/unlit_material.hpp"

namespace violet
{
static constexpr std::size_t DEBUG_DRAW_MAX_LINES = 1024ull * 64;

physics_debug::physics_debug(world& world)
    : m_world(world)
{
    m_geometry = std::make_unique<geometry>();
    m_geometry->add_attribute(
        "position",
        nullptr,
        DEBUG_DRAW_MAX_LINES * 2 * sizeof(vec3f),
        RHI_BUFFER_VERTEX | RHI_BUFFER_HOST_VISIBLE);
    m_geometry->add_attribute(
        "color",
        nullptr,
        DEBUG_DRAW_MAX_LINES * 2 * sizeof(vec3f),
        RHI_BUFFER_VERTEX | RHI_BUFFER_HOST_VISIBLE);
    m_geometry->set_indexes<std::uint32_t>(
        nullptr,
        DEBUG_DRAW_MAX_LINES * 2,
        RHI_BUFFER_INDEX | RHI_BUFFER_HOST_VISIBLE);
    m_material = std::make_unique<unlit_line_material>();

    m_object = world.create();
    world.add_component<transform_component, mesh_component, scene_component>(m_object);

    auto& mesh = m_world.get_component<mesh_component>(m_object);
    mesh.geometry = m_geometry.get();
    mesh.submeshes.push_back({
        .index_offset = 0,
        .index_count = 0,
        .material = m_material.get(),
    });
}

void physics_debug::tick()
{
    assert(m_geometry);

    std::memcpy(
        m_geometry->get_vertex_buffer("position")->get_buffer_pointer(),
        m_position.data(),
        m_position.size() * sizeof(vec3f));
    std::memcpy(
        m_geometry->get_vertex_buffer("color")->get_buffer_pointer(),
        m_color.data(),
        m_color.size() * sizeof(vec3f));
    std::memcpy(
        m_geometry->get_index_buffer()->get_buffer_pointer(),
        m_indexes.data(),
        m_indexes.size() * sizeof(std::uint32_t));

    auto& mesh = m_world.get_component<mesh_component>(m_object);
    mesh.submeshes[0].index_count = static_cast<std::uint32_t>(m_indexes.size());
}
} // namespace violet
