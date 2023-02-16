#pragma once

#include "scene/bounding_box.hpp"
#include <array>
#include <queue>
#include <stack>

namespace violet::scene
{
class bvh_tree
{
public:
    bvh_tree();

    std::size_t add(const bounding_volume_aabb& aabb);
    void remove(std::size_t proxy_id);

    void clear();

    std::size_t update(std::size_t proxy_id, const bounding_volume_aabb& aabb);

    bool visible(std::size_t proxy_id) const noexcept { return m_nodes[proxy_id].visible; }
    void frustum_culling(const std::array<math::float4, 6>& frustum);

    template <typename T>
    void print(T&& functor)
    {
        if (m_root_index == INVALID_NODE_INDEX)
            return;

        std::stack<std::size_t> dfs;
        dfs.push(m_root_index);

        while (!dfs.empty())
        {
            auto index = dfs.top();
            dfs.pop();

            functor(m_nodes[index].aabb, m_nodes[index].visible);

            if (m_nodes[index].depth > 0)
            {
                dfs.push(m_nodes[index].left_child);
                dfs.push(m_nodes[index].right_child);
            }
        }
    }

private:
    static constexpr std::size_t INVALID_NODE_INDEX = -1;

    struct bvh_node
    {
        bounding_volume_aabb aabb;

        std::size_t parent;
        std::size_t left_child;
        std::size_t right_child;

        int depth;

        bool visible;
    };

    void balance(std::size_t index);
    std::size_t rotate(std::size_t index);

    float calculate_cost(const bounding_volume_aabb& aabb);
    bounding_volume_aabb union_box(const bounding_volume_aabb& a, const bounding_volume_aabb& b);

    std::size_t allocate_node();
    void deallocate_node(std::size_t index);

    std::size_t m_root_index;

    std::deque<bvh_node> m_nodes;
    std::queue<std::size_t> m_free_nodes;
};
} // namespace violet::scene