#include "scene/bvh_tree.hpp"
#include "log.hpp"
#include <stack>

namespace ash::scene
{
static constexpr std::size_t INVALID_NODE_INDEX = -1;

bvh_tree::bvh_tree() : m_root_index(INVALID_NODE_INDEX)
{
}

std::size_t bvh_tree::add(const bounding_volume_aabb& aabb)
{
    std::size_t new_node_index = allocate_node();
    m_nodes[new_node_index].aabb = aabb;

    if (m_root_index == INVALID_NODE_INDEX)
    {
        m_nodes[new_node_index].parent = INVALID_NODE_INDEX;
        m_root_index = new_node_index;
        return new_node_index;
    }

    // Find the best sibling.
    std::size_t best_sibling_index = m_root_index;
    std::queue<std::pair<std::size_t, float>> bfs;
    bfs.push({m_root_index, 0.0f});
    float min_cost = std::numeric_limits<float>::lowest();
    while (!bfs.empty())
    {
        auto [sibling_index, inherited_cost] = bfs.front();
        bfs.pop();

        if (sibling_index == INVALID_NODE_INDEX)
            continue;

        float direct_cost = calculate_cost(union_box(m_nodes[sibling_index].aabb, aabb));
        if (direct_cost + inherited_cost < min_cost)
        {
            min_cost = direct_cost + inherited_cost;
            best_sibling_index = sibling_index;
        }
        inherited_cost += direct_cost - calculate_cost(m_nodes[sibling_index].aabb);

        if (calculate_cost(aabb) + inherited_cost < min_cost)
        {
            bfs.push({m_nodes[sibling_index].left_child, inherited_cost});
            bfs.push({m_nodes[sibling_index].right_child, inherited_cost});
        }
    }

    // Create a new parent.
    std::size_t old_parent_index = m_nodes[best_sibling_index].parent;
    std::size_t new_parent_index = allocate_node();

    m_nodes[new_parent_index].left_child = best_sibling_index;
    m_nodes[new_parent_index].right_child = new_node_index;
    m_nodes[new_parent_index].depth = m_nodes[best_sibling_index].depth + 1;
    m_nodes[best_sibling_index].parent = m_nodes[new_node_index].parent = new_parent_index;

    if (m_root_index == best_sibling_index)
    {
        m_nodes[new_parent_index].parent = INVALID_NODE_INDEX;
        m_root_index = new_parent_index;
    }
    else
    {
        m_nodes[new_parent_index].parent = old_parent_index;

        if (m_nodes[old_parent_index].left_child == best_sibling_index)
            m_nodes[old_parent_index].left_child = new_parent_index;
        else
            m_nodes[old_parent_index].right_child = new_parent_index;
    }

    // Walk back up the tree refitting AABBs.
    balance(m_nodes[new_node_index].parent);

    return new_node_index;
}

void bvh_tree::remove(std::size_t proxy_id)
{
    if (proxy_id == m_root_index)
    {
        m_root_index = INVALID_NODE_INDEX;
    }
    else
    {
        std::size_t parent_index = m_nodes[proxy_id].parent;
        std::size_t sibling_index = m_nodes[parent_index].left_child == proxy_id
                                        ? m_nodes[parent_index].right_child
                                        : m_nodes[parent_index].left_child;

        std::size_t grandparent_index = m_nodes[parent_index].parent;

        m_nodes[sibling_index].parent = grandparent_index;
        if (grandparent_index != INVALID_NODE_INDEX)
        {
            if (m_nodes[grandparent_index].left_child == parent_index)
                m_nodes[grandparent_index].left_child = sibling_index;
            else
                m_nodes[grandparent_index].right_child = sibling_index;

            balance(grandparent_index);
        }
        else
        {
            m_root_index = sibling_index;
        }

        deallocate_node(parent_index);
    }

    deallocate_node(proxy_id);
}

void bvh_tree::clear()
{
}

std::size_t bvh_tree::update(std::size_t proxy_id, const bounding_volume_aabb& aabb)
{
    remove(proxy_id);
    return add(aabb);
}

void bvh_tree::frustum_culling(const std::array<math::float4, 6>& frustum)
{
    if (m_root_index == -1)
        return;

    for (bvh_node& node : m_nodes)
        node.visible = false;

    std::stack<std::size_t> dfs;
    dfs.push(m_root_index);

    std::stack<std::size_t> inside_nodes;
    while (!dfs.empty())
    {
        std::size_t index = dfs.top();
        dfs.pop();

        bool cull = false;
        bool inside = true;
        for (std::size_t i = 0; i < 6; ++i)
        {
            math::float4_align max_vertex = {};
            math::float4_align min_vertex = {};
            if (frustum[i][0] < 0.0f)
            {
                max_vertex[0] = m_nodes[index].aabb.min[0];
                min_vertex[0] = m_nodes[index].aabb.max[0];
            }
            else
            {
                max_vertex[0] = m_nodes[index].aabb.max[0];
                min_vertex[0] = m_nodes[index].aabb.min[0];
            }

            if (frustum[i][1] < 0.0f)
            {
                max_vertex[1] = m_nodes[index].aabb.min[1];
                min_vertex[1] = m_nodes[index].aabb.max[1];
            }
            else
            {
                max_vertex[1] = m_nodes[index].aabb.max[1];
                min_vertex[1] = m_nodes[index].aabb.min[1];
            }

            if (frustum[i][2] < 0.0f)
            {
                max_vertex[2] = m_nodes[index].aabb.min[2];
                min_vertex[2] = m_nodes[index].aabb.max[2];
            }
            else
            {
                max_vertex[2] = m_nodes[index].aabb.max[2];
                min_vertex[2] = m_nodes[index].aabb.min[2];
            }

            math::float4_simd max = math::simd::load(max_vertex);
            math::float4_simd normal = math::simd::load(frustum[i]);
            if (math::vector_simd::dot(max, normal) + frustum[i][3] < 0.0f)
            {
                cull = true;
                break;
            }
            else if (inside)
            {
                math::float4_simd min = math::simd::load(min_vertex);
                if (math::vector_simd::dot(min, normal) + frustum[i][3] < 0.0f)
                    inside = false;
            }
        }

        if (!cull)
        {
            m_nodes[index].visible = true;
            if (inside)
            {
                inside_nodes.push(index);
            }
            else if (m_nodes[index].depth > 0)
            {
                dfs.push(m_nodes[index].left_child);
                dfs.push(m_nodes[index].right_child);
            }
        }
    }

    while (!inside_nodes.empty())
    {
        std::size_t index = inside_nodes.top();
        inside_nodes.pop();

        m_nodes[index].visible = true;
        if (m_nodes[index].depth > 0)
        {
            inside_nodes.push(m_nodes[index].left_child);
            inside_nodes.push(m_nodes[index].right_child);
        }
    }
}

void bvh_tree::balance(std::size_t index)
{
    while (index != INVALID_NODE_INDEX)
    {
        index = rotate(index);

        std::size_t child1_index = m_nodes[index].left_child;
        std::size_t child2_index = m_nodes[index].right_child;
        m_nodes[index].aabb = union_box(m_nodes[child1_index].aabb, m_nodes[child2_index].aabb);
        m_nodes[index].depth =
            std::max(m_nodes[child1_index].depth, m_nodes[child2_index].depth) + 1;

        index = m_nodes[index].parent;
    }
}

std::size_t bvh_tree::rotate(std::size_t index)
{
    std::size_t parent_index = m_nodes[index].parent;
    std::size_t left_index = m_nodes[index].left_child;
    std::size_t right_index = m_nodes[index].right_child;

    int balance = m_nodes[left_index].depth - m_nodes[right_index].depth;
    if (balance < -1)
    {
        m_nodes[right_index].parent = parent_index;
        if (parent_index != INVALID_NODE_INDEX)
        {
            if (m_nodes[parent_index].left_child == index)
                m_nodes[parent_index].left_child = right_index;
            else
                m_nodes[parent_index].right_child = right_index;
        }
        else
        {
            m_root_index = right_index;
        }

        std::size_t rl_index = m_nodes[right_index].left_child;
        std::size_t rr_index = m_nodes[right_index].right_child;
        if (m_nodes[rl_index].depth < m_nodes[rr_index].depth)
        {
            m_nodes[rl_index].parent = index;
            m_nodes[index].right_child = rl_index;
            m_nodes[right_index].left_child = index;
        }
        else
        {
            m_nodes[rr_index].parent = index;
            m_nodes[index].right_child = rr_index;
            m_nodes[right_index].right_child = index;
        }

        m_nodes[index].parent = right_index;

        m_nodes[index].aabb = union_box(
            m_nodes[m_nodes[index].left_child].aabb,
            m_nodes[m_nodes[index].right_child].aabb);
        m_nodes[index].depth = 1 + std::max(
                                       m_nodes[m_nodes[index].left_child].depth,
                                       m_nodes[m_nodes[index].right_child].depth);

        return right_index;
    }
    else if (balance > 1)
    {
        m_nodes[left_index].parent = parent_index;
        if (parent_index != INVALID_NODE_INDEX)
        {
            if (m_nodes[parent_index].left_child == index)
                m_nodes[parent_index].left_child = left_index;
            else
                m_nodes[parent_index].right_child = left_index;
        }
        else
        {
            m_root_index = left_index;
        }

        std::size_t ll_index = m_nodes[left_index].left_child;
        std::size_t lr_index = m_nodes[left_index].right_child;
        if (m_nodes[ll_index].depth < m_nodes[lr_index].depth)
        {
            m_nodes[ll_index].parent = index;
            m_nodes[index].left_child = ll_index;
            m_nodes[left_index].left_child = index;
        }
        else
        {
            m_nodes[lr_index].parent = index;
            m_nodes[index].left_child = lr_index;
            m_nodes[left_index].right_child = index;
        }

        m_nodes[index].parent = left_index;

        m_nodes[index].aabb = union_box(
            m_nodes[m_nodes[index].left_child].aabb,
            m_nodes[m_nodes[index].right_child].aabb);
        m_nodes[index].depth = 1 + std::max(
                                       m_nodes[m_nodes[index].left_child].depth,
                                       m_nodes[m_nodes[index].right_child].depth);

        return left_index;
    }

    return index;
}

float bvh_tree::calculate_cost(const bounding_volume_aabb& aabb)
{
    // SAH.
    math::float3 d = math::vector::sub(aabb.max, aabb.min);
    return d[0] * d[1] + d[1] * d[2] + d[2] * d[0];
}

bounding_volume_aabb bvh_tree::union_box(
    const bounding_volume_aabb& a,
    const bounding_volume_aabb& b)
{
    math::float4_simd min = math::simd::min(math::simd::load(a.min), math::simd::load(b.min));
    math::float4_simd max = math::simd::max(math::simd::load(a.max), math::simd::load(b.max));

    bounding_volume_aabb result;
    math::simd::store(min, result.min);
    math::simd::store(max, result.max);
    return result;
}

std::size_t bvh_tree::allocate_node()
{
    std::size_t result;
    if (!m_free_nodes.empty())
    {
        result = m_free_nodes.front();
        m_free_nodes.pop();

        m_nodes[result].parent = INVALID_NODE_INDEX;
        m_nodes[result].left_child = INVALID_NODE_INDEX;
        m_nodes[result].right_child = INVALID_NODE_INDEX;
        m_nodes[result].depth = 0;
    }
    else
    {
        result = m_nodes.size();
        m_nodes.push_back(
            {.parent = INVALID_NODE_INDEX,
             .left_child = INVALID_NODE_INDEX,
             .right_child = INVALID_NODE_INDEX,
             .depth = 0});
    }

    return result;
}

void bvh_tree::deallocate_node(std::size_t index)
{
    m_free_nodes.push(index);
}
} // namespace ash::scene