#include "editor/editor_ui.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "ui/ui.hpp"
#include "ui/ui_event.hpp"

namespace ash::editor
{
editor_ui::editor_ui()
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& event = system<core::event>();
    auto& ui = system<ui::ui>();

    // Create left container.
    m_left_container = std::make_unique<ui::element>();
    m_left_container->flex_grow(1.0f);
    m_left_container->link(ui.root());

    // Create scene view.
    m_scene_view = std::make_unique<scene_view>();
    m_scene_view->width_percent(100.0f);
    m_scene_view->height_percent(100.0f);
    m_scene_view->link(m_left_container.get());

    // Create right container.
    m_right_container = std::make_unique<ui::element>();
    m_right_container->width(300.0f);
    m_right_container->flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);
    m_right_container->link(ui.root());

    // Create hierarchy view.
    m_hierarchy_view = std::make_unique<hierarchy_view>();
    m_hierarchy_view->width_percent(100.0f);
    m_hierarchy_view->height_percent(50.0f);
    m_hierarchy_view->link(m_right_container.get());

    // Create component view.
    m_component_view = std::make_unique<component_view>();
    m_component_view->height_percent(50.0f);
    m_component_view->link(m_right_container.get());
}

void editor_ui::tick()
{
    m_scene_view->tick();
    ecs::entity selected_entity = m_hierarchy_view->selected_entity();
    m_component_view->show_component(selected_entity);
}
} // namespace ash::editor