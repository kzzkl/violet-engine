#pragma once

#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "ui/controls/dock_window.hpp"

namespace ash::editor
{
class component_panel_base : public ui::element
{
public:
    component_panel_base(std::string_view component_name);

    virtual void show_component(ecs::entity entity) = 0;

protected:
    std::unique_ptr<ui::panel> m_title;
    std::unique_ptr<ui::label> m_label;

    std::unique_ptr<element> m_container;
};

template <typename Component>
class component_panel : public component_panel_base
{
};

class component_view : public ui::dock_window
{
public:
    component_view(ui::dock_area* area, const ui::dock_window_theme& theme);

    void show_component(ecs::entity entity);

private:
    template <typename Component>
    void register_component()
    {
        ecs::component_id id = ecs::component_index::value<Component>();
        if (m_component_panels.size() <= id)
            m_component_panels.resize(id + 1);

        if (m_component_panels[id] == nullptr)
            m_component_panels[id] = std::make_unique<component_panel<Component>>();
    }

    void sync_component_panel(ecs::entity entity);

    ecs::entity m_current_entity;
    ecs::component_mask m_component_mask;

    std::vector<component_panel_base*> m_current_panels;
    std::vector<std::unique_ptr<component_panel_base>> m_component_panels;
};
} // namespace ash::editor