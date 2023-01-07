#include "editor/editor_ui.hpp"
#include "core/context.hpp"
#include "core/event.hpp"
#include "core/relation.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

namespace violet::editor
{
editor_ui::editor_ui()
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& event = system<core::event>();
    auto& ui = system<ui::ui>();
    auto& window = system<window::window>();

    auto window_extent = window.extent();

    m_dock_area = std::make_unique<ui::dock_area>(
        window_extent.width,
        window_extent.height,
        ui.theme<ui::dock_area_theme>("dark"));
    m_dock_area->width_percent(100.0f);
    m_dock_area->height_percent(100.0f);
    ui.root()->add(m_dock_area.get());

    auto& view_theme = ui.theme<ui::dock_window_theme>("dark");

    // Create scene view.
    m_scene_view = std::make_unique<scene_view>(m_dock_area.get(), view_theme);
    m_scene_view->width(window_extent.width);
    m_scene_view->height(window_extent.height);
    m_scene_view->name = "scene view";
    m_dock_area->dock(m_scene_view.get());

    // Create hierarchy view.
    m_hierarchy_view = std::make_unique<hierarchy_view>(m_dock_area.get(), view_theme);
    m_dock_area->dock(m_hierarchy_view.get(), m_scene_view.get(), ui::LAYOUT_EDGE_RIGHT);

    // Create component view.
    m_component_view = std::make_unique<component_view>(m_dock_area.get(), view_theme);
    m_dock_area->dock(m_component_view.get(), m_hierarchy_view.get(), ui::LAYOUT_EDGE_BOTTOM);
}

void editor_ui::tick()
{
    m_scene_view->tick();
    ecs::entity selected_entity = m_hierarchy_view->selected_entity();
    m_component_view->tick(selected_entity);
}
} // namespace violet::editor