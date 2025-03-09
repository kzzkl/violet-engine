#include "graphics/debug_drawer.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/materials/unlit_material.hpp"

namespace violet
{
static constexpr std::size_t DEBUG_DRAW_MAX_LINES = 1024ull * 128;

debug_drawer::debug_drawer(world& world)
    : m_world(world)
{
    m_geometry = std::make_unique<geometry>();
    m_geometry->set_position(std::vector<vec3f>(DEBUG_DRAW_MAX_LINES * 2));
    m_geometry->set_custom(0, std::vector<vec3f>(DEBUG_DRAW_MAX_LINES * 2)); // color
    m_geometry->set_indexes(std::vector<std::uint32_t>(DEBUG_DRAW_MAX_LINES * 2));
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

void debug_drawer::tick()
{
    assert(m_geometry);

    m_geometry->set_position(m_position);
    m_geometry->set_custom(0, m_color);
    m_geometry->set_indexes(m_indexes);

    auto& mesh = m_world.get_component<mesh_component>(m_object);
    mesh.submeshes[0].index_count = static_cast<std::uint32_t>(m_indexes.size());

    m_position.clear();
    m_color.clear();
    m_indexes.clear();
}
} // namespace violet
