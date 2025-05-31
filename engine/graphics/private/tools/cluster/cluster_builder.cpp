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

constexpr std::size_t MAX_BVH_CHILD_COUNT = 8;
constexpr std::size_t MAX_BVH_CHILD_BIT_COUNT = 3;

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

    std::vector<std::uint32_t> root_indexes;
    while (true)
    {
        auto cluster_offset = static_cast<std::uint32_t>(m_clusters.size());

        if (m_clusters.empty())
        {
            cluster_triangles(m_positions, m_indexes);
        }
        else
        {
            for (std::uint32_t i = group_offset; i < m_groups.size(); ++i)
            {
                simplify_group(i);
            }
        }

        auto cluster_count = static_cast<std::uint32_t>(m_clusters.size()) - cluster_offset;

        group_offset = static_cast<std::uint32_t>(m_groups.size());

        group_clusters(cluster_offset, cluster_count);

        for (std::uint32_t i = group_offset; i < m_groups.size(); ++i)
        {
            m_groups[i].lod = group_lod;
        }

        std::vector<std::uint32_t> group_indexes(m_groups.size() - group_offset);
        std::iota(group_indexes.begin(), group_indexes.end(), group_offset);
        root_indexes.push_back(build_bvh(group_indexes, false));

        ++group_lod;

        if (cluster_count == 1)
        {
            break;
        }
    }

    build_bvh(root_indexes, true);

    calculate_bvh_error();
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
    for (auto [begin, end] : parts)
    {
        cluster cluster = {
            .index_offset = begin * 3,
            .index_count = (end - begin) * 3,
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

                if (triangle_index_to_sorted[adjacency_triangle_index] < begin ||
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
            box::expand(cluster.bounding_box, positions[indexes[cluster.index_offset + i]]);
        }

        cluster.bounding_sphere.center = box::get_center(cluster.bounding_box);
        cluster.bounding_sphere.radius = 0.0f;
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
    if (cluster_count <= MAX_GROUP_SIZE)
    {
        cluster_group group = {
            .min_lod_error = std::numeric_limits<float>::infinity(),
            .max_parent_lod_error = std::numeric_limits<float>::infinity(),
            .cluster_offset = cluster_offset,
            .cluster_count = cluster_count,
        };

        std::vector<sphere3f> cluster_bounding_spheres;
        std::vector<sphere3f> cluster_lod_bounds;

        for (auto& cluster : clusters)
        {
            cluster.group_index = static_cast<std::uint32_t>(m_groups.size());

            group.min_lod_error = std::min(group.min_lod_error, cluster.lod_error);

            box::expand(group.bounding_box, cluster.bounding_box);
            cluster_bounding_spheres.push_back(cluster.bounding_sphere);
            cluster_lod_bounds.push_back(cluster.lod_bounds);
        }

        group.bounding_sphere = sphere::create(cluster_bounding_spheres);
        group.lod_bounds = sphere::create(cluster_lod_bounds);

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
    for (auto [begin, end] : parts)
    {
        cluster_group group = {
            .min_lod_error = std::numeric_limits<float>::infinity(),
            .max_parent_lod_error = std::numeric_limits<float>::infinity(),
            .cluster_offset = begin + cluster_offset,
            .cluster_count = end - begin,
        };

        std::vector<sphere3f> cluster_bounding_spheres;
        std::vector<sphere3f> cluster_lod_bounds;

        for (std::uint32_t cluster_index = begin; cluster_index < end; ++cluster_index)
        {
            cluster& cluster = clusters[cluster_index];
            cluster.group_index = static_cast<std::uint32_t>(m_groups.size());

            group.min_lod_error = std::min(group.min_lod_error, cluster.lod_error);

            box::expand(group.bounding_box, cluster.bounding_box);
            cluster_bounding_spheres.push_back(cluster.bounding_sphere);
            cluster_lod_bounds.push_back(cluster.lod_bounds);

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
                    if (adjacency_cluster_index < begin || adjacency_cluster_index >= end)
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

        group.bounding_sphere = sphere::create(cluster_bounding_spheres);
        group.lod_bounds = sphere::create(cluster_lod_bounds);

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

    for (std::size_t i = cluster_offset; i < m_clusters.size(); ++i)
    {
        m_clusters[i].index_offset += index_offset;
        m_clusters[i].lod_bounds = group.lod_bounds;
        m_clusters[i].lod_error = lod_error;
        m_clusters[i].child_group_index = group_index;
    }

    group.max_parent_lod_error = lod_error;
}

std::uint32_t cluster_builder::build_bvh(std::span<std::uint32_t> indexes, bool root)
{
    auto child_count = static_cast<std::uint32_t>(indexes.size());

    if (child_count == 1)
    {
        if (root)
        {
            return indexes[0];
        }

        auto node_index = static_cast<std::uint32_t>(m_bvh_nodes.size());
        auto& node = m_bvh_nodes.emplace_back();
        node.is_leaf = true;
        node.children.push_back(indexes[0]);
        return node_index;
    }

    if (child_count <= MAX_BVH_CHILD_COUNT)
    {
        std::vector<std::uint32_t> children;
        for (std::uint32_t i = 0; i < child_count; ++i)
        {
            children.push_back(build_bvh(indexes.subspan(i, 1), root));
        }

        auto node_index = static_cast<std::uint32_t>(m_bvh_nodes.size());
        auto& node = m_bvh_nodes.emplace_back();
        node.children = children;
        return node_index;
    }

    std::uint32_t large_child_count = MAX_BVH_CHILD_COUNT;
    while (large_child_count * MAX_BVH_CHILD_COUNT < child_count)
    {
        large_child_count *= MAX_BVH_CHILD_COUNT;
    }
    std::uint32_t small_child_count = large_child_count / MAX_BVH_CHILD_COUNT;
    std::uint32_t excess_child_count = child_count - small_child_count * MAX_BVH_CHILD_COUNT;

    std::array<std::uint32_t, 8> child_sizes;
    for (std::uint32_t i = 0; i < MAX_BVH_CHILD_COUNT; ++i)
    {
        std::uint32_t child_excess =
            std::min(large_child_count - small_child_count, excess_child_count);
        child_sizes[i] = small_child_count + child_excess;
        excess_child_count -= child_excess;
    }

    if (!root)
    {
        for (std::uint32_t i = 0; i < MAX_BVH_CHILD_BIT_COUNT; ++i)
        {
            std::uint32_t range_count = 1 << i;
            std::uint32_t range_size = MAX_BVH_CHILD_COUNT / range_count;

            std::uint32_t split_offset = 0;

            for (std::uint32_t j = 0; j < range_count; ++j)
            {
                std::uint32_t split0 = 0;
                std::uint32_t split1 = 0;
                for (std::uint32_t k = 0; k < range_size / 2; ++k)
                {
                    split0 += child_sizes[j * range_size + k];
                    split1 += child_sizes[j * range_size + k + range_size / 2];
                }

                sort_groups(indexes.subspan(split_offset, split0 + split1), split0);

                split_offset += split0 + split1;
            }
        }
    }

    std::vector<std::uint32_t> children;
    std::uint32_t offset = 0;
    for (std::uint32_t i = 0; i < MAX_BVH_CHILD_COUNT; ++i)
    {
        children.push_back(build_bvh(indexes.subspan(offset, child_sizes[i]), root));
        offset += child_sizes[i];
    }

    auto node_index = static_cast<std::uint32_t>(m_bvh_nodes.size());
    auto& node = m_bvh_nodes.emplace_back();
    node.children = children;
    return node_index;
}

void cluster_builder::sort_groups(std::span<std::uint32_t> group_indexes, std::uint32_t split)
{
    box3f bounding_box;
    for (std::uint32_t group_index : group_indexes)
    {
        const auto& group = m_groups[group_index];
        box::expand(bounding_box, group.bounding_box);
    }

    auto get_surface_area = [&](std::uint32_t begin, std::uint32_t end)
    {
        box3f bounding_box;
        for (std::uint32_t i = begin; i < end; ++i)
        {
            const auto& group = m_groups[group_indexes[i]];
            box::expand(bounding_box, group.bounding_box);
        }

        vec3f extent = box::get_extent(bounding_box);
        return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
    };

    float min_surface_area = std::numeric_limits<float>::infinity();
    std::uint32_t best_axis = 0;
    for (std::uint32_t axis = 0; axis < 3; ++axis)
    {
        std::sort(
            group_indexes.begin(),
            group_indexes.end(),
            [&](std::uint32_t a, std::uint32_t b) -> bool
            {
                vec3f a_center = box::get_center(m_groups[a].bounding_box);
                vec3f b_center = box::get_center(m_groups[b].bounding_box);
                return a_center[axis] < b_center[axis];
            });

        float surface_area =
            get_surface_area(0, split) +
            get_surface_area(split, static_cast<std::uint32_t>(group_indexes.size()));
        if (surface_area < min_surface_area)
        {
            min_surface_area = surface_area;
            best_axis = axis;
        }
    }

    std::sort(
        group_indexes.begin(),
        group_indexes.end(),
        [&](std::uint32_t a, std::uint32_t b) -> bool
        {
            vec3f a_center = box::get_center(m_groups[a].bounding_box);
            vec3f b_center = box::get_center(m_groups[b].bounding_box);
            return a_center[best_axis] < b_center[best_axis];
        });
}

void cluster_builder::calculate_bvh_error()
{
    for (auto iter = m_bvh_nodes.begin(); iter != m_bvh_nodes.end(); ++iter)
    {
        if (iter->is_leaf)
        {
            assert(iter->children.size() == 1);

            const auto& group = m_groups[iter->children[0]];

            iter->bounding_box = group.bounding_box;
            iter->bounding_sphere = group.bounding_sphere;
            iter->lod_bounds = group.lod_bounds;
            iter->min_lod_error = group.min_lod_error;
            iter->max_parent_lod_error = group.max_parent_lod_error;
        }
        else
        {
            iter->min_lod_error = std::numeric_limits<float>::infinity();
            iter->max_parent_lod_error = 0.0f;

            std::vector<sphere3f> child_bounding_spheres;
            std::vector<sphere3f> child_lod_bounds;

            for (std::uint32_t node_index : iter->children)
            {
                const auto& child = m_bvh_nodes[node_index];

                child_bounding_spheres.push_back(child.bounding_sphere);
                child_lod_bounds.push_back(child.lod_bounds);

                iter->min_lod_error = std::min(iter->min_lod_error, child.min_lod_error);
                iter->max_parent_lod_error =
                    std::max(iter->max_parent_lod_error, child.max_parent_lod_error);
            }

            iter->bounding_sphere = sphere::create(child_bounding_spheres);
            iter->lod_bounds = sphere::create(child_lod_bounds);
        }
    }
}
} // namespace violet