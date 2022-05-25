#include "ui/element_tree.hpp"
#include "core/context.hpp"
#include "core/link.hpp"
#include "ecs/world.hpp"
#include "ui/controls/container.hpp"

namespace ash::ui
{
element_tree::element_tree() : m_window_width(0.0f), m_window_height(0.0f)
{
    auto& world = system<ecs::world>();
    m_view = world.make_view<element>();

    m_root = world.create();
    world.add<core::link, element>(m_root);

    auto& root_element = world.component<element>(m_root);
    root_element.control = std::make_unique<container>();
    root_element.layout.flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    root_element.show = true;

    m_layout = std::make_unique<layout>(root_element.layout);
}

void element_tree::link(element& child, element& parent)
{
    child.layout.parent(&parent.layout);
}

bool element_tree::tick()
{
    auto& root_layout = system<ecs::world>().component<element>(m_root).layout;

    std::vector<element*> dirty_elements;
    m_view->each([&](element& element) {
        if (!element.show || element.control->type() == ELEMENT_CONTROL_TYPE_CONTAINER)
            return;
        if (element.layout.dirty())
            dirty_elements.push_back(&element);
    });

    if (dirty_elements.empty())
        return false;

    m_layout->calculate(&root_layout, m_window_width, m_window_height);

    for (auto& [key, value] : m_meshes)
    {
        value.vertex_position.clear();
        value.vertex_uv.clear();
        value.vertex_color.clear();
        value.indices.clear();
    }

    m_view->each([this](element& element) {
        if (!element.show || element.control->type() == ELEMENT_CONTROL_TYPE_CONTAINER)
            return;

        element_extent extent = element.layout.extent();
        element.control->extent(extent);

        auto& target_mesh =
            m_meshes[mesh_key{element.control->type(), element.control->mesh().texture}];
        auto& source_mesh = element.control->mesh();
        target_mesh.texture = element.control->mesh().texture;

        target_mesh.vertex_position.insert(
            target_mesh.vertex_position.end(),
            source_mesh.vertex_position.begin(),
            source_mesh.vertex_position.end());
        target_mesh.vertex_uv.insert(
            target_mesh.vertex_uv.end(),
            source_mesh.vertex_uv.begin(),
            source_mesh.vertex_uv.end());
        target_mesh.vertex_color.insert(
            target_mesh.vertex_color.end(),
            source_mesh.vertex_color.begin(),
            source_mesh.vertex_color.end());

        target_mesh.indices.insert(
            target_mesh.indices.end(),
            source_mesh.indices.begin(),
            source_mesh.indices.end());
    });

    return true;
}

void element_tree::resize(std::uint32_t width, std::uint32_t height)
{
    auto& world = system<ecs::world>();
    auto& root_element = world.component<element>(m_root);
    root_element.layout.resize(width, height);

    m_window_width = static_cast<float>(width);
    m_window_height = static_cast<float>(height);
}
} // namespace ash::ui