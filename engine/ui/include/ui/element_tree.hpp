#pragma once

#include "graphics_interface.hpp"
#include "ui/element.hpp"
#include <queue>
#include <stack>
#include <unordered_map>
#include <vector>

namespace ash::ui
{
class element_tree : public element
{
public:
    element_tree();

    virtual void tick() override;

    void resize_window(float window_width, float window_height);
    bool tree_dirty() const noexcept { return m_tree_dirty; }

private:
    void update_input();
    void update_layout();

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

    float m_window_width;
    float m_window_height;

    bool m_tree_dirty;
};
} // namespace ash::ui