#include "editor/editor_ui.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "ui/controls/container.hpp"
#include "ui/controls/plane.hpp"
#include "ui/ui.hpp"
#include "ui/ui_event.hpp"

namespace ash::editor
{
editor_ui::editor_ui()
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& event = system<core::event>();

    event.subscribe<ui::event_calculate_layout>("editor ui", [this]() { m_scene_view->resize(); });

    // Make main container.
    m_entity = world.create();
    world.add<core::link, ui::element>(m_entity);
    auto& main = world.component<ui::element>(m_entity);
    main.control = std::make_unique<ui::container>();
    main.layout.resize(100.0f, 100.0f, false, false, true, true);
    main.layout.direction(ui::LAYOUT_DIRECTION_LTR);
    main.layout.flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
    main.show = true;
    relation.link(m_entity, system<ui::ui>().root());

    // Make left container.
    m_left_container = world.create();
    world.add<core::link, ui::element>(m_left_container);
    auto& left = world.component<ui::element>(m_left_container);
    left.control = std::make_unique<ui::plane>(ui::COLOR_BISQUE);
    left.layout.resize(0.0f, 0.0f, true, true);
    left.layout.flex_grow(1.0f);
    left.show = true;
    relation.link(m_left_container, m_entity);

    m_scene_view = std::make_unique<scene_view>(m_left_container);

    // Make right container.
    m_right_container = world.create();
    world.add<core::link, ui::element>(m_right_container);
    auto& right = world.component<ui::element>(m_right_container);
    right.control = std::make_unique<ui::plane>(ui::COLOR_AQUA);
    right.layout.resize(300.0f, 0.0f, false, true);
    right.layout.flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);
    right.show = true;
    relation.link(m_right_container, m_entity);

    m_hierarchy_view = std::make_unique<hierarchy_view>(m_right_container);
}

void editor_ui::tick()
{
    m_scene_view->tick();
}
} // namespace ash::editor