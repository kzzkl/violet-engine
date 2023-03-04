#pragma once

#include "graphics_interface.hpp"
#include "ui/controls/view.hpp"
#include <queue>
#include <vector>

namespace violet::ui
{
class control_tree : public view
{
public:
    control_tree();

    void tick(float width, float height);
    bool tree_dirty() const noexcept { return m_tree_dirty; }

    void input(char c) { m_input_chars.push(c); }

private:
    void update_input();
    void update_layout(float width, float height);

    void bubble_mouse_event(control* hot_node, control* focused_node, control* drag_node);

    virtual void on_remove_child(control* child) override;

    template <typename Functor>
    void bfs(control* root, Functor&& functor)
    {
        std::queue<control*> bfs;
        bfs.push(root);

        while (!bfs.empty())
        {
            control* node = bfs.front();
            bfs.pop();

            if (functor(node))
            {
                for (control* child : node->children())
                    bfs.push(child);
            }
        }
    }

    control* m_hot_node;
    control* m_focused_node;
    control* m_drag_node;

    std::vector<control*> m_mouse_over_nodes;

    std::queue<char> m_input_chars;

    bool m_tree_dirty;
};
} // namespace violet::ui