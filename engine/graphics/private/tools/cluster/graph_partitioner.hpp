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
        std::size_t min_count,
        std::size_t max_count);

    std::span<const std::uint32_t> get_vertices() const
    {
        return m_vertices;
    }

    std::span<const std::pair<std::size_t, std::size_t>> get_parts() const
    {
        return m_parts;
    }

private:
    std::size_t bisect_graph(
        std::size_t start,
        std::size_t end,
        std::size_t min_count,
        std::size_t max_count);

    void partition_recursive(
        std::size_t start,
        std::size_t end,
        std::size_t min_count,
        std::size_t max_count);

    std::vector<std::uint32_t> m_vertices;
    std::vector<std::size_t> m_vertex_map;

    std::span<const std::uint32_t> m_adjacency;
    std::span<const std::uint32_t> m_adjacency_cost;
    std::span<const std::uint32_t> m_adjacency_offset;

    std::vector<std::pair<std::size_t, std::size_t>> m_parts;
};
} // namespace violet