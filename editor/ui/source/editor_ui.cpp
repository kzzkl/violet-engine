#include "editor/editor_ui.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "ui/ui.hpp"
#include "ui/ui_event.hpp"
#include "window/window.hpp"

namespace ash::editor
{
editor_ui::editor_ui()
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& event = system<core::event>();
    auto& ui = system<ui::ui>();
    auto& window = system<window::window>();

    auto window_extent = window.extent();

    // Create scene view.
    m_scene_view = std::make_unique<scene_view>();
    m_scene_view->width(window_extent.width);
    m_scene_view->height(window_extent.height);
    //m_scene_view->sync_extent();
    m_scene_view->name = "scene view";
    // m_scene_view->m_width = window_extent.width;
    // m_scene_view->m_height = window_extent.height;
    m_scene_view->link(ui.root());

    // Create hierarchy view.
    m_hierarchy_view = std::make_unique<hierarchy_view>();
    m_hierarchy_view->dock(m_scene_view.get(), ui::LAYOUT_EDGE_RIGHT);

    // Create component view.
    m_component_view = std::make_unique<component_view>();
    m_component_view->dock(m_hierarchy_view.get(), ui::LAYOUT_EDGE_BOTTOM);
}

void print_tree(ui::element* node, std::size_t block = 0)
{
    auto d = dynamic_cast<ui::dock_element*>(node);
    if (d != nullptr)
    {
        std::string b(block, '-');
        auto& e = node->extent();
        log::debug(
            "|{} {}({}, {}) ({} {} {} {})",
            b,
            node->name,
            d->m_width,
            d->m_height,
            e.x,
            e.y,
            e.width,
            e.height);
    }
    else
    {
        std::string b(block, '-');
        auto& e = node->extent();
        log::debug(
            "|{} {}({} {} {} {})",
            b,
            node->name,
            e.x,
            e.y,
            e.width,
            e.height);
    }

    for (ui::element* child : node->children())
    {
        print_tree(child, block + 5);
    }
}

void editor_ui::tick()
{
    m_scene_view->tick();
    ecs::entity selected_entity = m_hierarchy_view->selected_entity();
    m_component_view->show_component(selected_entity);

    if (system<window::window>().keyboard().key(window::KEYBOARD_KEY_A).press())
        print_tree(system<ui::ui>().root());
}
} // namespace ash::editor