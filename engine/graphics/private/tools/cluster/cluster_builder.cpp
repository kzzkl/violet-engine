#include "tools/cluster/cluster_builder.hpp"
#include "algorithm/disjoint_set.hpp"
#include "algorithm/hash.hpp"
#include "tools/cluster/graph_linker.hpp"
#include "tools/cluster/graph_partitioner.hpp"
#include "tools/mesh_simplifier/mesh_simplifier.hpp"
#include <iterator>
#include <map>
#include <unordered_map>

namespace violet
{
namespace
{
constexpr std::size_t MAX_CLUSTER_SIZE = 128;

constexpr std::size_t MAX_GROUP_SIZE = 32;
constexpr std::size_t MIN_GROUP_SIZE = 8;

std::uint32_t cycle_3(std::uint32_t value)
{
    std::uint32_t value_mod3 = value % 3;
    std::uint32_t value1_mod3 = (1 << value_mod3) & 3;
    return value - value_mod3 + value1_mod3;
}

struct vertex_hash
{
    std::uint32_t operator()(const vec3f& v) const noexcept
    {
        union
        {
            float f;
            std::uint32_t i;
        } x, y, z;

        x.f = v.x;
        y.f = v.y;
        z.f = v.z;

        return hash::murmur_32({x.i, y.i, z.i});
    }
};

struct edge_key
{
    vec3f p0;
    vec3f p1;

    bool operator==(const edge_key& other) const noexcept
    {
        return p0 == other.p0 && p1 == other.p1;
    }
};

struct edge_key_hash
{
    std::uint32_t operator()(const edge_key& key) const noexcept
    {
        vertex_hash vertex_hash;
        std::uint32_t p0_hash = vertex_hash(key.p0);
        std::uint32_t p1_hash = vertex_hash(key.p1);
        return hash::murmur_32({p0_hash, p1_hash});
    }
};

template <typename T>
using edge_map = std::unordered_multimap<edge_key, T, edge_key_hash>;
} // namespace

void cluster_builder::set_positions(std::span<const vec3f> positions)
{
    m_bounds = {};
    for (const vec3f& position : positions)
    {
        box::expand(m_bounds, position);
    }

    m_positions.assign(positions.begin(), positions.end());
}

void cluster_builder::set_indexes(std::span<const std::uint32_t> indexes)
{
    m_indexes.assign(indexes.begin(), indexes.end());
}

void cluster_builder::build()
{
    std::uint32_t group_offset = 0;
    std::uint32_t group_lod = 0;
    while (true)
    {
        auto cluster_offset = static_cast<std::uint32_t>(m_clusters.size());

        if (m_clusters.empty())
        {
            cluster_triangles(m_positions, m_indexes);
        }
        else
        {
            while (group_offset < m_groups.size())
            {
                simplify_group(group_offset);
                ++group_offset;
            }
        }

        auto cluster_count = static_cast<std::uint32_t>(m_clusters.size()) - cluster_offset;

        group_clusters(cluster_offset, cluster_count);

        for (std::uint32_t i = group_offset; i < m_groups.size(); ++i)
        {
            m_groups[i].lod = group_lod;
        }

        ++group_lod;

        if (cluster_count == 1)
        {
            break;
        }
    }
}

void cluster_builder::cluster_triangles(
    const std::vector<vec3f>& positions,
    std::vector<std::uint32_t>& indexes)
{
    auto edge_count = static_cast<std::uint32_t>(indexes.size());
    std::uint32_t triangle_count = edge_count / 3;

    disjoint_set<std::uint32_t> disjoint_set(triangle_count);

    // Build a half-edge hash table for subsequent rapid lookup of adjacent edges.
    edge_map<std::uint32_t> edge_map;
    for (std::uint32_t edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        edge_key edge_key = {
            .p0 = positions[indexes[edge_index]],
            .p1 = positions[indexes[cycle_3(edge_index)]],
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
            .p0 = positions[indexes[cycle_3(edge_index)]],
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

    auto vertices = partitioner.get_vertices();
    auto parts = partitioner.get_parts();

    // Construct a new index buffer based on the sorted triangles.
    std::vector<std::uint32_t> new_indexes(indexes.size());
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        new_indexes[i * 3 + 0] = indexes[vertices[i] * 3 + 0];
        new_indexes[i * 3 + 1] = indexes[vertices[i] * 3 + 1];
        new_indexes[i * 3 + 2] = indexes[vertices[i] * 3 + 2];
    }
    indexes = std::move(new_indexes);

    // Establish a mapping from sorted triangles to original triangles to associate new indices with
    // their predecessors, enabling reuse of edge adjacency tables.
    std::vector<std::uint32_t> triangle_index_to_sorted(vertices.size());
    for (std::uint32_t i = 0; i < vertices.size(); ++i)
    {
        triangle_index_to_sorted[vertices[i]] = i;
    }

    // Construct clusters based on the sorted triangles.
    m_clusters.reserve(m_clusters.size() + parts.size());
    for (auto [start, end] : parts)
    {
        cluster cluster = {
            .index_offset = start * 3,
            .index_count = (end - start) * 3,
        };

        // Find the external edges of the cluster.
        cluster.external_edges.resize(cluster.index_count);
        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            std::uint32_t triangle_index = vertices[(cluster.index_offset + i) / 3];
            std::uint32_t edge_index = triangle_index * 3 + i % 3;

            // If the edge is adjacent to an edge in another cluster, it must be an external edge.
            for (std::size_t j = edge_adjacency_offset[edge_index];
                 j < edge_adjacency_offset[edge_index + 1];
                 ++j)
            {
                std::uint32_t adjacency_edge_index = edge_adjacency[j];
                std::uint32_t adjacency_triangle_index = adjacency_edge_index / 3;

                if (triangle_index_to_sorted[adjacency_triangle_index] < start ||
                    triangle_index_to_sorted[adjacency_triangle_index] >= end)
                {
                    cluster.external_edges[i] |= cluster::EXTERNAL_EDGE_CLUSTER;
                }
            }

            // If the edge is not adjacent to any other edges, it must be an external edge.
            if (edge_adjacency_offset[edge_index] == edge_adjacency_offset[edge_index + 1])
            {
                cluster.external_edges[i] |= cluster::EXTERNAL_EDGE_CLUSTER;
            }
        }

        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            sphere::expand(cluster.bounding_sphere, positions[indexes[cluster.index_offset + i]]);
        }
        cluster.lod_bounds = cluster.bounding_sphere;

        m_clusters.push_back(cluster);
    }
}

void cluster_builder::group_clusters(std::uint32_t cluster_offset, std::uint32_t cluster_count)
{
    std::span<cluster> clusters(m_clusters.data() + cluster_offset, cluster_count);

    // If the number of clusters is small, no need to group.
    if (clusters.size() <= MAX_GROUP_SIZE)
    {
        cluster_group group = {
            .cluster_offset = cluster_offset,
            .cluster_count = cluster_count,
        };

        m_groups.push_back(group);

        return;
    }

    // Build a half-edge hash table for subsequent rapid lookup of adjacent clusters.
    edge_map<std::uint32_t> edge_map;
    for (std::uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
    {
        const cluster& cluster = clusters[cluster_index];

        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            if (cluster.external_edges[i] & cluster::EXTERNAL_EDGE_CLUSTER)
            {
                std::uint32_t edge_index = cluster.index_offset + i;

                edge_key edge_key = {
                    .p0 = m_positions[m_indexes[edge_index]],
                    .p1 = m_positions[m_indexes[cycle_3(edge_index)]],
                };
                edge_map.insert({edge_key, cluster_index});
            }
        }
    }

    // Construct cluster adjacency relationships.
    // Key: adjacency cluster index.
    // Value: adjacency edge count. Used to determine the cost of grouping two clusters.
    std::vector<std::map<std::uint32_t, std::uint32_t>> cluster_adjacency_map(clusters.size());
    for (std::uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
    {
        const auto& cluster = clusters[cluster_index];

        for (std::uint32_t i = 0; i < cluster.index_count; ++i)
        {
            // If the edge is not an external edge, it is not an adjacency edge.
            if (cluster.external_edges[i] == 0)
            {
                continue;
            }

            std::uint32_t edge_index = cluster.index_offset + i;

            auto range = edge_map.equal_range({
                .p0 = m_positions[m_indexes[cycle_3(edge_index)]],
                .p1 = m_positions[m_indexes[edge_index]],
            });
            for (auto iter = range.first; iter != range.second; ++iter)
            {
                ++cluster_adjacency_map[cluster_index][iter->second];
            }
        }
    }

    // Build a disjoint set for linking independent clusters.
    disjoint_set<std::uint32_t> disjoint_set(static_cast<std::uint32_t>(clusters.size()));
    for (std::uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
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
            return box::get_center(clusters[cluster_index].bounding_box);
        });

    // Partition the clusters into groups.
    std::vector<std::uint32_t> cluster_adjacency;
    std::vector<std::uint32_t> cluster_adjacency_cost;
    std::vector<std::uint32_t> cluster_adjacency_offset;
    cluster_adjacency_offset.reserve(clusters.size() + 1);
    for (std::uint32_t cluster_index = 0; cluster_index < clusters.size(); ++cluster_index)
    {
        cluster_adjacency_offset.push_back(static_cast<std::uint32_t>(cluster_adjacency.size()));

        for (auto [adjacency_cluster_index, adjacency_count] : cluster_adjacency_map[cluster_index])
        {
            cluster_adjacency.push_back(adjacency_cluster_index);
            cluster_adjacency_cost.push_back(adjacency_count * 16 + 4);
        }

        // The bridging edges previously added to ensure connectivity must also be factored in.
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

    // Reorder the clusters based on the sorted clusters.
    auto vertices = partitioner.get_vertices();
    auto parts = partitioner.get_parts();
    std::vector<cluster> new_clusters(vertices.size());
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        new_clusters[i] = std::move(clusters[vertices[i]]);
    }
    for (std::size_t i = 0; i < vertices.size(); ++i)
    {
        clusters[i] = std::move(new_clusters[i]);
    }

    std::vector<std::uint32_t> cluster_index_to_sorted(vertices.size());
    for (std::uint32_t i = 0; i < vertices.size(); ++i)
    {
        cluster_index_to_sorted[vertices[i]] = i;
    }

    // Construct groups based on the sorted clusters.
    m_groups.reserve(parts.size());
    for (auto [start, end] : parts)
    {
        cluster_group group = {
            .cluster_offset = start + cluster_offset,
            .cluster_count = end - start,
        };

        for (std::uint32_t cluster_index = start; cluster_index < end; ++cluster_index)
        {
            cluster& cluster = clusters[cluster_index];

            // Find the external edges of the group.
            for (std::uint32_t i = 0; i < cluster.index_count; ++i)
            {
                if (cluster.external_edges[i] == 0)
                {
                    continue;
                }

                std::uint32_t edge_index = cluster.index_offset + i;

                auto range = edge_map.equal_range({
                    .p0 = m_positions[m_indexes[cycle_3(edge_index)]],
                    .p1 = m_positions[m_indexes[edge_index]],
                });

                // If the edge is adjacent to an edge in another cluster not in the group, it must
                // be an external edge.
                for (auto iter = range.first; iter != range.second; ++iter)
                {
                    std::uint32_t adjacency_cluster_index = cluster_index_to_sorted[iter->second];
                    if (adjacency_cluster_index < start || adjacency_cluster_index >= end)
                    {
                        cluster.external_edges[i] |= cluster::EXTERNAL_EDGE_GROUP;
                    }
                }

                // If the edge is not adjacent to any other edges, it must be an external edge.
                if (range.first == range.second)
                {
                    cluster.external_edges[i] |= cluster::EXTERNAL_EDGE_GROUP;
                }
            }
        }

        m_groups.push_back(group);
    }
}

void cluster_builder::simplify_group(std::uint32_t group_index)
{
    auto& group = m_groups[group_index];

    // Collect the indexes of the group for simplification.
    std::vector<std::uint32_t> group_indexes;
    for (std::uint32_t i = 0; i < group.cluster_count; ++i)
    {
        const cluster& cluster = m_clusters[group.cluster_offset + i];

        group_indexes.insert(
            group_indexes.end(),
            m_indexes.begin() + cluster.index_offset,
            m_indexes.begin() + cluster.index_offset + cluster.index_count);
    }

    float lod_error = 0.0f;

    mesh_simplifier simplifier;
    simplifier.set_positions(m_positions);
    simplifier.set_indexes(group_indexes);

    for (std::uint32_t i = 0; i < group.cluster_count; ++i)
    {
        const cluster& cluster = m_clusters[group.cluster_offset + i];

        // Lock the positions of the external edges of the group.
        for (std::uint32_t j = 0; j < cluster.index_count; ++j)
        {
            if (cluster.external_edges[j] & cluster::EXTERNAL_EDGE_GROUP)
            {
                simplifier.lock_position(m_positions[m_indexes[cluster.index_offset + j]]);
            }
        }

        lod_error = std::max(lod_error, cluster.lod_error);
    }

    float error = simplifier.simplify(static_cast<std::uint32_t>(group_indexes.size() / 3 / 2));
    std::vector<vec3f> new_positions = simplifier.get_positions();
    std::vector<std::uint32_t> new_indexes = simplifier.get_indexes();

    lod_error = std::max(lod_error, error);

    std::size_t cluster_offset = m_clusters.size();

    // Cluster the new triangles.
    cluster_triangles(new_positions, new_indexes);

    auto position_offset = static_cast<std::uint32_t>(m_positions.size());
    m_positions.insert(m_positions.end(), new_positions.begin(), new_positions.end());

    auto index_offset = static_cast<std::uint32_t>(m_indexes.size());
    std::transform(
        new_indexes.begin(),
        new_indexes.end(),
        std::back_inserter(m_indexes),
        [=](std::uint32_t index) -> std::uint32_t
        {
            return index + position_offset;
        });

    sphere3f lod_bounds;
    for (std::uint32_t i = 0; i < group.cluster_count; ++i)
    {
        sphere::expand(lod_bounds, m_clusters[group.cluster_offset + i].lod_bounds);
    }

    for (std::size_t i = cluster_offset; i < m_clusters.size(); ++i)
    {
        m_clusters[i].index_offset += index_offset;
        m_clusters[i].lod_bounds = lod_bounds;
        m_clusters[i].lod_error = lod_error;
        m_clusters[i].children_group = group_index;
    }

    for (std::uint32_t i = 0; i < group.cluster_count; ++i)
    {
        cluster& cluster = m_clusters[group.cluster_offset + i];
        cluster.parent_lod_bounds = lod_bounds;
        cluster.parent_lod_error = lod_error;
    }
}
} // namespace violet