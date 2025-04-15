#include "tools/cluster/cluster_builder.hpp"
#include "algorithm/disjoint_set.hpp"
#include "algorithm/hash.hpp"
#include "tools/cluster/graph_linker.hpp"
#include "tools/cluster/graph_partitioner.hpp"
#include <map>
#include <unordered_map>

namespace violet
{
namespace
{
constexpr std::size_t MAX_CLUSTER_SIZE = 128;

constexpr std::size_t MAX_GROUP_SIZE = 32;
constexpr std::size_t MIN_GROUP_SIZE = 8;

std::uint32_t cycle3(std::uint32_t value)
{
    std::uint32_t value_mod3 = value % 3;
    std::uint32_t value1_mod3 = (1 << value_mod3) & 3;
    return value - value_mod3 + value1_mod3;
}

struct edge_key
{
    vec3f p0;
    vec3f p1;

    bool operator==(const edge_key& other) const noexcept
    {
        return p0 == other.p0 && p1 == other.p1;
    }
};

class edge_key_hash
{
public:
    std::size_t operator()(const edge_key& key) const noexcept
    {
        std::uint32_t p0_hash = hash_position(key.p0);
        std::uint32_t p1_hash = hash_position(key.p1);
        return hash::murmur_32({p0_hash, p1_hash});
    }

private:
    std::uint32_t hash_position(const vec3f& position) const noexcept
    {
        union
        {
            float f;
            std::uint32_t i;
        } x, y, z;

        x.f = position.x;
        y.f = position.y;
        z.f = position.z;

        return hash::murmur_32({x.i, y.i, z.i});
    }
};

template <typename T>
using edge_map = std::unordered_multimap<edge_key, T, edge_key_hash>;
} // namespace

void cluster_builder::build(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes)
{
    for (const vec3f& position : positions)
    {
        box::expand(m_bounds, position);
    }

    cluster_triangles(positions, indexes);
    build_cluster_groups(positions);
}

void cluster_builder::cluster_triangles(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes)
{
    std::size_t edge_count = indexes.size();
    std::size_t triangle_count = edge_count / 3;

    disjoint_set<std::uint32_t> disjoint_set(triangle_count);

    // Build a half-edge hash table for subsequent rapid lookup of adjacent edges.
    edge_map<std::uint32_t> edge_map;
    for (std::uint32_t edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        edge_key edge_key = {
            .p0 = positions[indexes[edge_index]],
            .p1 = positions[indexes[cycle3(edge_index)]],
        };
        edge_map.insert({edge_key, edge_index});
    }

    // Construct edge adjacency relationships for subsequent building of triangle adjacency
    // relationships.
    std::vector<std::uint32_t> edge_adjacency;
    std::vector<std::uint32_t> edge_adjacency_offset;
    edge_adjacency_offset.reserve(edge_count + 1);
    for (std::uint32_t edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        edge_adjacency_offset.push_back(static_cast<std::uint32_t>(edge_adjacency.size()));

        auto range = edge_map.equal_range({
            .p0 = positions[indexes[cycle3(edge_index)]],
            .p1 = positions[indexes[edge_index]],
        });
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            edge_adjacency.push_back(iter->second);
            disjoint_set.merge(edge_index / 3, iter->second / 3);
        }
    }
    edge_adjacency_offset.push_back(static_cast<std::uint32_t>(edge_adjacency.size()));

    // Add bridging edges between disconnected components to ensure the overall connectedness of the
    // mesh.
    graph_linker linker;
    linker.build_external_links(
        disjoint_set,
        MAX_CLUSTER_SIZE,
        m_bounds,
        [&](std::uint32_t triangle_index) -> vec3f
        {
            vec3f center = positions[indexes[triangle_index * 3 + 0]];
            center += positions[indexes[triangle_index * 3 + 1]];
            center += positions[indexes[triangle_index * 3 + 2]];
            center /= 3.0f;

            return center;
        });

    // Establish triangle adjacency relationships.
    std::vector<std::uint32_t> triangle_adjacency;
    std::vector<std::uint32_t> triangle_adjacency_cost;
    std::vector<std::uint32_t> triangle_adjacency_offset;
    triangle_adjacency_offset.reserve(triangle_count + 1);
    for (std::uint32_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
    {
        triangle_adjacency_offset.push_back(static_cast<std::uint32_t>(triangle_adjacency.size()));

        for (std::uint32_t i = 0; i < 3; ++i)
        {
            std::uint32_t edge_index = triangle_index * 3 + i;

            for (std::size_t j = edge_adjacency_offset[edge_index];
                 j < edge_adjacency_offset[edge_index + 1];
                 ++j)
            {
                std::uint32_t adjacency_edge_index = edge_adjacency[j];
                std::uint32_t adjacency_triangle_index = adjacency_edge_index / 3;

                triangle_adjacency.push_back(adjacency_triangle_index);
                triangle_adjacency_cost.push_back(4 * 65);
            }
        }

        // The bridging edges previously added to ensure connectivity must also be factored in.
        linker.get_external_link(triangle_index, triangle_adjacency, triangle_adjacency_cost);
    }
    triangle_adjacency_offset.push_back(static_cast<std::uint32_t>(triangle_adjacency.size()));

    // Partition the mesh into clusters.
    graph_partitioner partitioner;
    partitioner.partition(
        triangle_adjacency,
        triangle_adjacency_cost,
        triangle_adjacency_offset,
        MAX_CLUSTER_SIZE - 4,
        MAX_CLUSTER_SIZE);

    auto triangles = partitioner.get_vertices();
    auto parts = partitioner.get_parts();

    // Construct a new index buffer based on the sorted triangles.
    m_indexes.reserve(indexes.size());
    for (std::uint32_t triangle_index : triangles)
    {
        m_indexes.push_back(indexes[triangle_index * 3 + 0]);
        m_indexes.push_back(indexes[triangle_index * 3 + 1]);
        m_indexes.push_back(indexes[triangle_index * 3 + 2]);
    }

    // Establish a mapping from sorted triangles to original triangles to associate new indices with
    // their predecessors, enabling reuse of edge adjacency tables.
    std::vector<std::uint32_t> triangle_index_to_sorted(triangles.size());
    for (std::uint32_t i = 0; i < triangles.size(); ++i)
    {
        triangle_index_to_sorted[triangles[i]] = i;
    }

    // Construct clusters based on the sorted triangles.
    m_clusters.reserve(parts.size());
    for (auto [start, end] : parts)
    {
        cluster cluster = {
            .index_offset = static_cast<std::uint32_t>(start * 3),
            .index_count = static_cast<std::uint32_t>((end - start) * 3),
        };

        // Count the number of external edges.
        cluster.external_edges.resize(cluster.index_count);
        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            std::uint32_t triangle_index = triangles[(cluster.index_offset + i) / 3];
            std::uint32_t edge_index = triangle_index * 3 + i % 3;

            for (std::size_t j = edge_adjacency_offset[edge_index];
                 j < edge_adjacency_offset[edge_index + 1];
                 ++j)
            {
                std::uint32_t adjacency_edge_index = edge_adjacency[j];
                std::uint32_t adjacency_triangle_index = adjacency_edge_index / 3;

                if (triangle_index_to_sorted[adjacency_triangle_index] < start ||
                    triangle_index_to_sorted[adjacency_triangle_index] >= end)
                {
                    ++cluster.external_edges[i];
                }
            }
        }

        m_clusters.push_back(cluster);
    }
}

void cluster_builder::build_cluster_groups(std::span<const vec3f> positions)
{
    edge_map<std::uint32_t> edge_map;
    for (std::uint32_t cluster_index = 0; cluster_index < m_clusters.size(); ++cluster_index)
    {
        const cluster& cluster = m_clusters[cluster_index];

        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            if (cluster.external_edges[i] != 0)
            {
                std::uint32_t edge_index = cluster.index_offset + i;

                edge_key edge_key = {
                    .p0 = positions[m_indexes[edge_index]],
                    .p1 = positions[m_indexes[cycle3(edge_index)]],
                };
                edge_map.insert({edge_key, cluster_index});
            }
        }
    }

    // key: adjacency cluster index, value: adjacency edge count.
    std::vector<std::map<std::uint32_t, std::uint32_t>> cluster_adjacency_map(m_clusters.size());
    for (std::uint32_t cluster_index = 0; cluster_index < m_clusters.size(); ++cluster_index)
    {
        const auto& cluster = m_clusters[cluster_index];

        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            if (cluster.external_edges[i] == 0)
            {
                continue;
            }

            std::uint32_t edge_index = cluster.index_offset + i;

            auto range = edge_map.equal_range({
                .p0 = positions[m_indexes[cycle3(edge_index)]],
                .p1 = positions[m_indexes[edge_index]],
            });
            for (auto iter = range.first; iter != range.second; ++iter)
            {
                ++cluster_adjacency_map[cluster_index][iter->second];
            }
        }
    }

    disjoint_set<std::uint32_t> disjoint_set(m_clusters.size());
    for (std::uint32_t cluster_index = 0; cluster_index < m_clusters.size(); ++cluster_index)
    {
        for (auto [adjacency_cluster_index, adjacency_count] : cluster_adjacency_map[cluster_index])
        {
            disjoint_set.merge(cluster_index, adjacency_cluster_index);
        }
    }

    graph_linker linker;
    linker.build_external_links(
        disjoint_set,
        MAX_GROUP_SIZE,
        m_bounds,
        [&](std::uint32_t cluster_index) -> vec3f
        {
            return box::get_center(m_clusters[cluster_index].bounds);
        });

    std::vector<std::uint32_t> cluster_adjacency;
    std::vector<std::uint32_t> cluster_adjacency_cost;
    std::vector<std::uint32_t> cluster_adjacency_offset;
    cluster_adjacency_offset.reserve(m_clusters.size() + 1);
    for (std::uint32_t cluster_index = 0; cluster_index < m_clusters.size(); ++cluster_index)
    {
        cluster_adjacency_offset.push_back(static_cast<std::uint32_t>(cluster_adjacency.size()));

        for (auto [adjacency_cluster_index, adjacency_count] : cluster_adjacency_map[cluster_index])
        {
            cluster_adjacency.push_back(adjacency_cluster_index);
            cluster_adjacency_cost.push_back(adjacency_count * 16 + 4);
        }

        linker.get_external_link(cluster_index, cluster_adjacency, cluster_adjacency_cost);
    }
    cluster_adjacency_offset.push_back(static_cast<std::uint32_t>(cluster_adjacency.size()));

    graph_partitioner partitioner;
    partitioner.partition(
        cluster_adjacency,
        cluster_adjacency_cost,
        cluster_adjacency_offset,
        MIN_GROUP_SIZE,
        MAX_GROUP_SIZE);

    auto clusters = partitioner.get_vertices();
    auto parts = partitioner.get_parts();

    m_groups.reserve(parts.size());
    for (auto [start, end] : parts)
    {
        cluster_group group;
        group.clusters.reserve(end - start);

        for (std::uint32_t i = start; i < end; ++i)
        {
            group.clusters.push_back(clusters[i]);
        }

        m_groups.push_back(group);
    }
}

void cluster_builder::simplify_cluster_group(cluster_group& group) {}
} // namespace violet