#include "graphics/render_graph/rdg_profiling.hpp"
#include <cassert>

namespace violet
{
rdg_profiling::rdg_profiling()
{
    auto& device = render_device::instance();
    m_query_pools.resize(device.get_frame_resource_count());
}

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
    m_query_pool_index = (m_query_pool_index + 1) % m_query_pools.size();

    m_nodes.clear();

    while (!m_node_stack.empty())
    {
        m_node_stack.pop();
    }

    auto& query_pool = m_query_pools[m_query_pool_index];

    std::uint32_t required_size = leaf_count + 1;
    if (query_pool == nullptr || required_size > query_pool->get_size())
    {
        std::uint32_t size = 1;
        while (size < required_size)
        {
            size <<= 1;
        }

        query_pool = render_device::instance().create_query_pool({
            .type = RHI_QUERY_TYPE_TIMESTAMP,
            .size = size,
        });
    }

    query_pool->reset();

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
    get_query_pool()->get_results(results.data(), static_cast<std::uint32_t>(results.size()));

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

rhi_query_pool* rdg_profiling::get_query_pool() const
{
    return m_query_pools[m_query_pool_index].get();
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