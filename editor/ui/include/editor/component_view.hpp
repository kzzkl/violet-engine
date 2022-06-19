#pragma once

#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "ui/controls/collapse.hpp"
#include "ui/controls/dock_window.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
class component_panel : public ui::collapse
{
public:
    component_panel(std::string_view component_name, const ui::collapse_theme& theme);

    virtual void tick(bool entity_change, ecs::entity entity) = 0;
};

class component_view : public ui::dock_window
{
public:
    component_view(ui::dock_area* area, const ui::dock_window_theme& theme);

    void tick(ecs::entity entity);

private:
    template <typename Component, typename ComponentPanel>
    void register_component()
    {
        ecs::component_id id = ecs::component_index::value<Component>();
        if (m_component_panels.size() <= id)
            m_component_panels.resize(id + 1);

        if (m_component_panels[id] == nullptr)
        {
            m_component_panels[id] = std::make_unique<ComponentPanel>(
                system<ui::ui>().theme<ui::collapse_theme>("dark"));
            m_component_panels[id]->margin(5.0f, ui::LAYOUT_EDGE_BOTTOM);
            m_component_panels[id]->hide();
            add_item(m_component_panels[id].get());
        }
    }

    void sync_component_panel(ecs::entity entity);

    ecs::entity m_current_entity;
    ecs::component_mask m_component_mask;

    std::vector<component_panel*> m_current_panels;
    std::vector<std::unique_ptr<component_panel>> m_component_panels;
};
} // namespace ash::editor