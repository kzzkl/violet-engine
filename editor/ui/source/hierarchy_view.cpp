#include "hierarchy_view.hpp"
#include "link.hpp"

namespace ash::editor
{
hierarchy_view::hierarchy_view(ecs::entity scene_root, ecs::world& world)
    : m_scene_root(scene_root),
      m_world(world)
{
}

void hierarchy_view::draw(ui::ui& ui, editor_data& data)
{
    ui.window("hierarchy");
    draw_node(ui, m_scene_root, data);
    ui.window_pop();
}

void hierarchy_view::draw_node(ui::ui& ui, ecs::entity entity, editor_data& data)
{
    auto& info = m_world.component<ecs::information>(entity);
    auto& link = m_world.component<core::link>(entity);

    auto [open, clicked] = ui.tree_ex(info.name, link.children.empty());
    if (clicked)
        data.active_entity = entity;

    if (open)
    {
        for (ecs::entity child : link.children)
            draw_node(ui, child, data);

        ui.tree_pop();
    }
}
} // namespace ash::editor
