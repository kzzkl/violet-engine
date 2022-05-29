#pragma once

#include "ecs/entity.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"

namespace ash::editor
{
class hierarchy_node : public ui::element
{
public:
    hierarchy_node(ecs::entity entity, ecs::entity* selected);

    void reset(ecs::entity entity);

    element* container() const noexcept { return m_container.get(); }

private:
    std::unique_ptr<ui::panel> m_title;
    std::unique_ptr<ui::label> m_label;

    std::unique_ptr<element> m_container;

    ecs::entity m_entity;
    ecs::entity* m_selected;
};

class hierarchy_view : public ui::panel
{
public:
    hierarchy_view();
    virtual ~hierarchy_view();

    ecs::entity selected_entity() const noexcept { return m_selected; }

private:
    hierarchy_node* get_or_create_node(ecs::entity entity);

    ecs::entity m_selected;
    std::vector<std::unique_ptr<hierarchy_node>> m_node_pool;
};
} // namespace ash::editor