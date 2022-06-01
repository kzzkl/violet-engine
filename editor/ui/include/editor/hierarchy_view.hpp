#pragma once

#include "ecs/entity.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/controls/scroll_view.hpp"
#include <queue>

namespace ash::editor
{
class hierarchy_view : public ui::scroll_view
{
public:
    hierarchy_view();
    virtual ~hierarchy_view();

    ecs::entity selected_entity() const noexcept { return m_selected; }

private:
    class hierarchy_node : public ui::element
    {
    public:
        hierarchy_node(hierarchy_view* view);

        void reset(ecs::entity entity);

        element* container() const noexcept { return m_container.get(); }

    private:
        void expand();
        void collapse();

        std::unique_ptr<ui::panel> m_title;
        std::unique_ptr<ui::label> m_label;

        std::unique_ptr<element> m_container;

        ecs::entity m_entity;
        hierarchy_view* m_view;
    };

    hierarchy_node* allocate_node();
    void deallocate_node(hierarchy_node* node);

    ecs::entity m_selected;

    std::queue<hierarchy_node*> m_free_node;
    std::vector<std::unique_ptr<hierarchy_node>> m_node_pool;
};
} // namespace ash::editor