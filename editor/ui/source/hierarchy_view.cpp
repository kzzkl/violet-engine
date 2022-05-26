#include "editor/hierarchy_view.hpp"
#include "core/context.hpp"
#include "core/relation.hpp"
#include "ui/controls/plane.hpp"
#include "ui/element.hpp"

namespace ash::editor
{
hierarchy_view::hierarchy_view(ecs::entity ui_parent)
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();

    m_ui = world.create();
    world.add<core::link, ui::element>(m_ui);

    auto& hierarchy = world.component<ui::element>(m_ui);
    hierarchy.control = std::make_unique<ui::plane>(ui::COLOR_BLUE_VIOLET);
    hierarchy.layout.resize(0.0f, 50.0f, true, false, false, true);
    hierarchy.show = true;

    relation.link(m_ui, ui_parent);
}

hierarchy_view::~hierarchy_view()
{
}
} // namespace ash::editor