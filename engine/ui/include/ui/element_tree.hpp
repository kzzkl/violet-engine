#pragma once

#include "graphics_interface.hpp"
#include "ui/controls/dock_window.hpp"
#include "ui/controls/view.hpp"
#include <queue>
#include <stack>
#include <unordered_map>
#include <vector>

namespace ash::ui
{
class element_tree : public view
{
public:
    element_tree();

    void tick(float width, float height);
    bool tree_dirty() const noexcept { return m_tree_dirty; }

private:
    void update_input();
    // void update_docking();
    void update_layout(float width, float height);

    void bubble_mouse_event(element* hot_node);

    virtual void on_remove_child(element* child) override;

    template <typename Functor>
    void bfs(element* root, Functor&& functor)
    {
        std::queue<element*> bfs;
        bfs.push(root);

        while (!bfs.empty())
        {
            element* node = bfs.front();
            bfs.pop();

            if (functor(node))
            {
                for (element* child : node->children())
                    bfs.push(child);
            }
        }
    }

    element* m_hot_node;
    element* m_focused_node;
    element* m_drag_node;

    dock_element* m_dock_node;

    std::vector<element*> m_mouse_over_nodes;

    bool m_tree_dirty;
};
} // namespace ash::ui