#include "tools/cluster/graph_partitioner.hpp"
#include "metis.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <mutex>
#include <numeric>

namespace violet
{
void graph_partitioner::partition(
    std::span<const std::uint32_t> adjacency,
    std::span<const std::uint32_t> adjacency_cost,
    std::span<const std::uint32_t> adjacency_offset,
    std::uint32_t min_count,
    std::uint32_t max_count)
{
    auto vertex_count = static_cast<std::uint32_t>(adjacency_offset.size() - 1);

    m_vertices.resize(vertex_count);
    std::iota(m_vertices.begin(), m_vertices.end(), 0);

    if (vertex_count <= max_count)
    {
        m_parts.emplace_back(0, vertex_count);
        return;
    }

    m_vertex_map.resize(vertex_count);
    std::iota(m_vertex_map.begin(), m_vertex_map.end(), 0);

    m_adjacency = adjacency;
    m_adjacency_cost = adjacency_cost;
    m_adjacency_offset = adjacency_offset;

    partition_recursive(0, vertex_count, min_count, max_count);
}

std::uint32_t graph_partitioner::bisect_graph(
    std::uint32_t start,
    std::uint32_t end,
    std::uint32_t min_count,
    std::uint32_t max_count)
{
    std::vector<idx_t> adjacency;
    std::vector<idx_t> adjacency_cost;
    std::vector<idx_t> adjacency_offset;

    for (std::uint32_t i = start; i < end; ++i)
    {
        adjacency_offset.push_back(static_cast<idx_t>(adjacency.size()));

        for (std::uint32_t j = m_adjacency_offset[m_vertices[i]];
             j < m_adjacency_offset[m_vertices[i] + 1];
             ++j)
        {
            std::uint32_t adjacency_index = m_vertex_map[m_adjacency[j]];
            if (adjacency_index >= start && adjacency_index < end)
            {
                adjacency.push_back(static_cast<idx_t>(adjacency_index - start));
                adjacency_cost.push_back(static_cast<idx_t>(m_adjacency_cost[j]));
            }
        }
    }
    adjacency_offset.push_back(static_cast<idx_t>(adjacency.size()));

    idx_t options[METIS_NOPTIONS];
    METIS_SetDefaultOptions(options);
    options[METIS_OPTION_UFACTOR] = 200;

    auto vertex_count = static_cast<idx_t>(adjacency_offset.size() - 1);
    idx_t part_count = 2;
    idx_t constraints_count = 1;
    idx_t edges_cut = 0;

    auto target_part_count = static_cast<std::uint32_t>(std::ceil(
        static_cast<float>(end - start) / (static_cast<float>(min_count + max_count) / 2.0f)));
    target_part_count = std::max(2u, target_part_count);

    std::uint32_t left_part_count = target_part_count / 2;

    real_t weights[2] = {
        static_cast<float>(left_part_count) / static_cast<float>(target_part_count),
        1.0f - (static_cast<float>(left_part_count) / static_cast<float>(target_part_count)),
    };

    std::vector<idx_t> parts(vertex_count);

    {
        // METIS_PartGraphRecursive may crash when called from multiple threads. The root cause is
        // currently unknown.
        static std::mutex mutex;
        std::scoped_lock lock(mutex);

        int result = METIS_PartGraphRecursive(
            &vertex_count,
            &constraints_count,
            adjacency_offset.data(),
            adjacency.data(),
            nullptr,
            nullptr,
            adjacency_cost.data(),
            &part_count,
            weights,
            nullptr,
            options,
            &edges_cut,
            parts.data());
        assert(result == METIS_OK);
    }

    std::uint32_t left = 0;
    std::uint32_t right = end - start - 1;

    while (left < right)
    {
        while (parts[left] == 0)
        {
            ++left;
        }

        while (parts[right] == 1)
        {
            --right;
        }

        if (left < right)
        {
            std::swap(parts[left], parts[right]);
            std::swap(m_vertices[start + left], m_vertices[start + right]);
            m_vertex_map[m_vertices[start + left]] = start + left;
            m_vertex_map[m_vertices[start + right]] = start + right;
        }
    }

    return start + left;
}

void graph_partitioner::partition_recursive(
    std::uint32_t start,
    std::uint32_t end,
    std::uint32_t min_count,
    std::uint32_t max_count)
{
    std::uint32_t split = bisect_graph(start, end, min_count, max_count);

    if (split - start > max_count)
    {
        partition_recursive(start, split, min_count, max_count);
    }
    else
    {
        m_parts.emplace_back(start, split);
    }

    if (end - split > max_count)
    {
        partition_recursive(split, end, min_count, max_count);
    }
    else
    {
        m_parts.emplace_back(split, end);
    }
}
} // namespace violet