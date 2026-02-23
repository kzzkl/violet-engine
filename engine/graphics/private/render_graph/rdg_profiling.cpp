#include "graphics/render_graph/rdg_profiling.hpp"
#include <cassert>

namespace violet
{
void rdg_profiling::begin(std::string_view name, std::uint32_t query_index)
{
    auto index = static_cast<std::uint32_t>(m_nodes.size());

    if (!m_node_stack.empty())
    {
        m_nodes[m_node_stack.top()].children.push_back(index);
    }
    m_node_stack.push(index);

    auto& node = m_nodes.emplace_back();
    node.name = name;
    node.query_index = query_index;
}

void rdg_profiling::end()
{
    m_node_stack.pop();
}

void rdg_profiling::reset(std::uint32_t leaf_count)
{
    m_nodes.clear();

    while (!m_node_stack.empty())
    {
        m_node_stack.pop();
    }

    if (m_query_pool == nullptr || leaf_count > m_query_pool->get_size())
    {
        std::uint32_t size = 1;
        while (size < leaf_count)
        {
            size <<= 1;
        }

        m_query_pool = render_device::instance().create_query_pool({
            .type = RHI_QUERY_TYPE_TIMESTAMP,
            .size = size,
        });
    }

    m_query_pool->reset();

    m_leaf_count = leaf_count;
    m_resolved = false;
}

void rdg_profiling::resolve()
{
    if (m_resolved)
    {
        return;
    }

    assert(m_node_stack.empty());

    std::vector<std::uint64_t> results(m_leaf_count + 1);
    m_query_pool->get_results(results.data(), static_cast<std::uint32_t>(results.size()));

    for (auto& node : m_nodes)
    {
        if (node.query_index != 0xFFFFFFFF)
        {
            std::uint64_t delta_ns = results[node.query_index] - results[node.query_index - 1];
            node.time_ms = static_cast<float>(delta_ns) * 1e-6f;
        }
    }

    get_node_time(0);

    m_resolved = true;
}

float rdg_profiling::get_node_time(std::uint32_t node_index)
{
    auto& node = m_nodes[node_index];
    if (node.query_index != 0xFFFFFFFF)
    {
        return node.time_ms;
    }

    node.time_ms = 0.0f;
    for (auto child_index : node.children)
    {
        node.time_ms += get_node_time(child_index);
    }

    return node.time_ms;
}
} // namespace violet