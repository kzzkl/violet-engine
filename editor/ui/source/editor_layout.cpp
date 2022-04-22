#include "editor_layout.hpp"
#include "component_view.hpp"
#include "hierarchy_view.hpp"
#include "render_view.hpp"
#include "scene.hpp"

namespace ash::editor
{
class root_view : public editor_view
{
public:
    virtual void draw(ui::ui& ui, editor_data& data) override {}
};

editor_layout::editor_layout() : core::system_base("editor_layout")
{
    m_data.active_entity = ecs::INVALID_ENTITY;
}

bool editor_layout::initialize(const dictionary& config)
{
    auto& world = system<ecs::world>();
    world.register_component<editor_ui>();

    m_ui_root = world.create("editor_root");
    world.add<core::link, editor_ui>(m_ui_root);

    auto& view = world.component<editor_ui>(m_ui_root);
    view.show = true;
    view.interface = std::make_unique<root_view>();

    initialize_layout();

    return true;
}

void editor_layout::draw()
{
    auto& relation = system<core::relation>();
    auto& world = system<ecs::world>();
    auto& ui = system<ui::ui>();

    ui.window_root("root");
    relation.each_dfs(m_ui_root, [&](ecs::entity entity) {
        auto& information = world.component<ecs::information>(entity);
        auto& view = world.component<editor_ui>(entity);
        if (view.show)
        {
            view.interface->draw(ui, m_data);
            return true;
        }
        else
        {
            return false;
        }
    });
    ui.window_pop();
}

void editor_layout::initialize_layout()
{
    auto& world = system<ecs::world>();

    // Hierarchy view.
    create_view<hierarchy_view>("hierarchy", m_ui_root, system<scene::scene>().root(), world);

    // Component view.
    create_view<component_view>("component", m_ui_root, world);

    // Render view.
    create_view<render_view>("render", m_ui_root);
}
} // namespace ash::editor