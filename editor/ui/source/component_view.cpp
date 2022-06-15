#include "editor/component_view.hpp"
#include "core/context.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
component_panel_base::component_panel_base(std::string_view component_name)
{
    auto& ui = system<ui::ui>();

    m_title = std::make_unique<ui::panel>();
    m_title->height(40.0f);
    m_title->padding(10.0f, ui::LAYOUT_EDGE_LEFT);
    m_title->on_mouse_press = [this](window::mouse_key key, int x, int y) {
        if (m_container->display())
            m_container->hide();
        else
            m_container->show();
        return false;
    };
    m_title->link(this);

    m_label = std::make_unique<ui::label>(component_name, ui.theme<ui::label_theme>("dark"));
    m_label->link(m_title.get());

    m_container = std::make_unique<ui::panel>(ui::COLOR_VIOLET);
    m_container->flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);
    m_container->padding(20.0f, ui::LAYOUT_EDGE_LEFT);
    m_container->hide();
    m_container->link(this);
}

template <>
class component_panel<ecs::information> : public component_panel_base
{
public:
    component_panel<ecs::information>() : component_panel_base("Information")
    {
        auto& ui = system<ui::ui>();
        ui::label_theme label_theme = {};
        label_theme.text_font = &ui.font(ui::DEFAULT_TEXT_FONT);
        m_name = std::make_unique<ui::label>("none", label_theme);
        m_name->width(100.0f);
        m_name->height(30.0f);
        m_name->link(m_container.get());
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

component_view::component_view(ui::dock_area* area, const ui::dock_window_theme& theme)
    : ui::dock_window("Component", 0xF161, area, theme),
      m_current_entity(ecs::INVALID_ENTITY)
{
    register_component<ecs::information>();
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

    for (auto& component_panel : m_current_panels)
        component_panel->show_component(entity);

    m_current_entity = entity;
}

void component_view::sync_component_panel(ecs::entity entity)
{
    auto& world = system<ecs::world>();

    for (auto component_panel : m_current_panels)
        remove(component_panel);
    m_current_panels.clear();

    if (entity == ecs::INVALID_ENTITY)
        return;

    for (auto component : world.components(entity))
    {
        if (component < m_component_panels.size() && m_component_panels[component] != nullptr)
        {
            m_current_panels.push_back(m_component_panels[component].get());
            add(m_component_panels[component].get());
        }
    }
}
} // namespace ash::editor