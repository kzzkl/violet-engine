#include "hierarchy_view.hpp"
#include "link.hpp"
#include "scene.hpp"
#include "ui.hpp"

namespace ash::editor
{
hierarchy_view::hierarchy_view(core::context* context) : editor_view(context)
{
}

void hierarchy_view::draw(editor_data& data)
{
    auto& ui = system<ui::ui>();
    auto& scene = system<scene::scene>();
    auto& world = system<ecs::world>();

    ui.window("Hierarchy");
    auto& link = world.component<core::link>(scene.root());
    for (ecs::entity child : link.children)
        draw_node(child, data);
    ui.window_pop();
}

void hierarchy_view::draw_node(ecs::entity entity, editor_data& data)
{
    auto& ui = system<ui::ui>();
    auto& world = system<ecs::world>();

    auto& info = world.component<ecs::information>(entity);
    auto& link = world.component<core::link>(entity);

    auto [open, clicked] = ui.tree_ex(info.name, link.children.empty());
    if (clicked)
        data.active_entity = entity;

    if (open)
    {
        for (ecs::entity child : link.children)
            draw_node(child, data);

        ui.tree_pop();
    }
}
} // namespace ash::editor
