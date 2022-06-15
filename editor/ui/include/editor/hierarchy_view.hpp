#pragma once

#include "ecs/entity.hpp"
#include "ui/controls/dock_window.hpp"
#include "ui/controls/tree.hpp"
#include <map>
#include <queue>

namespace ash::editor
{
class hierarchy_view : public ui::dock_window
{
public:
    hierarchy_view(ui::dock_area* area, const ui::dock_window_theme& theme);
    virtual ~hierarchy_view();

    ecs::entity selected_entity() const noexcept { return m_selected; }

private:
    void load_entity(ecs::entity entity);
    void unload_entity(ecs::entity entity);

    ui::tree_node* allocate_node(ecs::entity entity, bool loaded);
    void deallocate_node(ecs::entity entity);

    ecs::entity m_selected;

    std::unique_ptr<ui::tree> m_tree;
    std::queue<ui::tree_node*> m_free_node;
    std::vector<std::unique_ptr<ui::tree_node>> m_node_pool;

    struct entity_node
    {
        ui::tree_node* node;
        bool loaded;
    };

    std::map<ecs::entity, entity_node> m_entity_to_node;
    std::map<ui::tree_node*, ecs::entity> m_node_to_entity;

    const ui::font* m_text_font;
};
} // namespace ash::editor