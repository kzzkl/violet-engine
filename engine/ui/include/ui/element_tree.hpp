#pragma once

#include "graphics_interface.hpp"
#include "ui/controls/view.hpp"
#include <queue>
#include <vector>

namespace violet::ui
{
class element_tree : public view
{
public:
    element_tree();

    void tick(float width, float height);
    bool tree_dirty() const noexcept { return m_tree_dirty; }

    void input(char c) { m_input_chars.push(c); }

private:
    void update_input();
    void update_layout(float width, float height);

    void bubble_mouse_event(element* hot_node, element* focused_node, element* drag_node);

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

    std::vector<element*> m_mouse_over_nodes;

    std::queue<char> m_input_chars;

    bool m_tree_dirty;
};
} // namespace violet::ui