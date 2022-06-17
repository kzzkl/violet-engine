#include "editor/component_view.hpp"
#include "core/context.hpp"
#include "scene/transform.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
component_panel::component_panel(std::string_view component_name, const ui::collapse_theme& theme)
    : ui::collapse(component_name, theme)
{
    container()->padding(10.0f, ui::LAYOUT_EDGE_ALL);
}

class information_panel : public component_panel
{
public:
    information_panel(const ui::collapse_theme& theme) : component_panel("Information", theme)
    {
        auto& ui = system<ui::ui>();
        m_name = std::make_unique<ui::label>("none", ui.theme<ui::label_theme>("dark"));
        m_name->width(100.0f);
        m_name->height(30.0f);
        add_item(m_name.get());
    }

    virtual void show_component(ecs::entity entity) override
    {
        auto& world = system<ecs::world>();
        auto& info = world.component<ecs::information>(entity);
        m_name->text(std::string("name: ") + info.name);
    }

private:
    std::unique_ptr<ui::label> m_name;
};

class transform_panel : public component_panel
{
public:
    transform_panel(const ui::collapse_theme& theme) : component_panel("Transform", theme) {}

    virtual void show_component(ecs::entity entity) override {}

private:
    std::unique_ptr<ui::panel> m_property_panel;
};

component_view::component_view(ui::dock_area* area, const ui::dock_window_theme& theme)
    : ui::dock_window("Component", 0xF161, area, theme),
      m_current_entity(ecs::INVALID_ENTITY)
{
    register_component<ecs::information, information_panel>();
    register_component<scene::transform, transform_panel>();
}

void component_view::show_component(ecs::entity entity)
{
    if (m_current_entity == entity)
        return;

    auto& world = system<ecs::world>();
    if (m_component_mask != world.mask(entity))
    {
        sync_component_panel(entity);
        m_component_mask = world.mask(entity);
    }

    m_current_entity = entity;
}

void component_view::sync_component_panel(ecs::entity entity)
{
    auto& world = system<ecs::world>();

    for (auto component_panel : m_current_panels)
        component_panel->hide();
    m_current_panels.clear();

    if (entity == ecs::INVALID_ENTITY)
        return;

    for (auto component : world.components(entity))
    {
        if (component < m_component_panels.size() && m_component_panels[component] != nullptr)
        {
            m_component_panels[component]->show_component(entity);
            m_component_panels[component]->show();
            m_current_panels.push_back(m_component_panels[component].get());
        }
    }
}
} // namespace ash::editor