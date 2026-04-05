#pragma once

#include <span>
#include <vector>

namespace violet
{
class graph_partitioner
{
public:
    void partition(
        std::span<const std::uint32_t> adjacency,
        std::span<const std::uint32_t> adjacency_cost,
        std::span<const std::uint32_t> adjacency_offset,
        std::uint32_t min_count,
        std::uint32_t max_count);

    const std::vector<std::uint32_t>& get_vertices() const
    {
        return m_vertices;
    }

    const std::vector<std::pair<std::uint32_t, std::uint32_t>>& get_parts() const
    {
        return m_parts;
    }

private:
    std::uint32_t bisect_graph(
        std::uint32_t start,
        std::uint32_t end,
        std::uint32_t min_count,
        std::uint32_t max_count);

    void partition_recursive(
        std::uint32_t start,
        std::uint32_t end,
        std::uint32_t min_count,
        std::uint32_t max_count);

    std::vector<std::uint32_t> m_vertices;
    std::vector<std::uint32_t> m_vertex_map;

    std::span<const std::uint32_t> m_adjacency;
    std::span<const std::uint32_t> m_adjacency_cost;
    std::span<const std::uint32_t> m_adjacency_offset;

    std::vector<std::pair<std::uint32_t, std::uint32_t>> m_parts;
};
} // namespace violet