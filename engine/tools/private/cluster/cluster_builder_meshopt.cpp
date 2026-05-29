#include "cluster/cluster_builder_meshopt.hpp"
#include "math/vector.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <limits>
#include <meshoptimizer.h>
#include <numeric>
#include <unordered_map>

namespace violet
{
namespace
{
constexpr std::uint32_t MAX_BVH_CHILD_COUNT = 8;
constexpr std::uint32_t MAX_BVH_CHILD_BIT_COUNT = 3;
constexpr float SIMPLIFY_ERROR_FACTOR_SLOPPY = 2.0f;

struct sloppy_vertex
{
    float x;
    float y;
    float z;
    std::uint32_t id;
};
} // namespace

void cluster_builder_meshopt::set_positions(std::span<const vec3f> positions)
{
    m_bounds = {};
    m_positions.resize(positions.size());

    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        m_positions[i].x = std::isnan(positions[i].x) ? 0.0f : positions[i].x;
        m_positions[i].y = std::isnan(positions[i].y) ? 0.0f : positions[i].y;
        m_positions[i].z = std::isnan(positions[i].z) ? 0.0f : positions[i].z;

        box::expand(m_bounds, m_positions[i]);
    }
}

void cluster_builder_meshopt::set_normals(std::span<const vec3f> normals)
{
    m_normals.resize(normals.size());

    for (std::size_t i = 0; i < normals.size(); ++i)
    {
        if (std::isnan(normals[i].x) || std::isnan(normals[i].y) || std::isnan(normals[i].z))
        {
            m_normals[i] = {.x = 0.0f, .y = 1.0f, .z = 0.0f};
        }
        else
        {
            m_normals[i] = normals[i];
        }
    }
}

void cluster_builder_meshopt::set_tangents(std::span<const vec4f> tangents)
{
    m_tangents.resize(tangents.size());

    for (std::size_t i = 0; i < tangents.size(); ++i)
    {
        if (std::isnan(tangents[i].x) || std::isnan(tangents[i].y) || std::isnan(tangents[i].z))
        {
            m_tangents[i] = {.x = 0.0f, .y = 1.0f, .z = 0.0f, .w = tangents[i].w};
        }
        else
        {
            m_tangents[i] = tangents[i];
        }
    }
}

void cluster_builder_meshopt::set_texcoords(std::span<const vec2f> texcoords)
{
    m_texcoords.resize(texcoords.size());

    for (size_t i = 0; i < texcoords.size(); ++i)
    {
        m_texcoords[i].x = std::isnan(texcoords[i].x) ? 0.0f : texcoords[i].x;
        m_texcoords[i].y = std::isnan(texcoords[i].y) ? 0.0f : texcoords[i].y;
    }
}

void cluster_builder_meshopt::set_indexes(std::span<const std::uint32_t> indexes)
{
    m_indexes.assign(indexes.begin(), indexes.end());
}

void cluster_builder_meshopt::build(const cluster_builder_meshopt_options& options)
{
    m_options = options;
    m_clusters.clear();
    m_cluster_nodes.clear();
    m_groups.clear();

    if (m_positions.empty() || m_indexes.empty())
    {
        return;
    }

    std::vector<std::uint32_t> source_indexes = m_indexes;
    m_indexes.clear();

    std::vector<std::uint8_t> locks;
    std::vector<std::uint32_t> remap;

    auto update_remap_and_locks = [&]()
    {
        locks.assign(m_positions.size(), 0);
        remap.resize(m_positions.size());
        meshopt_generatePositionRemap(
            remap.data(),
            &m_positions[0].x,
            m_positions.size(),
            sizeof(vec3f));

        if (m_texcoords.empty())
        {
            return;
        }

        for (std::size_t i = 0; i < m_positions.size(); ++i)
        {
            std::uint32_t r = remap[i];
            if (r != i && m_texcoords[i] != m_texcoords[r])
            {
                locks[i] |= meshopt_SimplifyVertex_Protect;
            }
        }
    };

    update_remap_and_locks();

    std::vector<meshopt_cluster> clusters =
        clusterize(&m_positions[0].x, sizeof(vec3f), source_indexes.data(), source_indexes.size());
    for (meshopt_cluster& cluster : clusters)
    {
        compute_cluster_bounds(cluster, 0.0f);
        cluster.lod_bounds = cluster.bounding_sphere;
    }

    std::vector<int> pending(clusters.size());
    std::iota(pending.begin(), pending.end(), 0);

    std::uint32_t lod = 0;
    while (pending.size() > 1)
    {
        update_remap_and_locks();

        std::vector<std::vector<int>> groups = partition(clusters, pending, remap);
        pending.clear();

        lock_boundaries(locks, groups, clusters, remap);

        for (const auto& group : groups)
        {
            std::vector<std::uint32_t> merged;
            merged.reserve(group.size() * m_options.max_triangles * 3);
            for (int cluster_index : group)
            {
                const meshopt_cluster& cluster = clusters[cluster_index];
                merged.insert(merged.end(), cluster.indices.begin(), cluster.indices.end());
            }

            std::size_t target_index_count = merged.size() / 3;
            target_index_count =
                static_cast<std::size_t>(
                    static_cast<float>(target_index_count) * m_options.simplify_ratio) *
                3;
            target_index_count = std::max<std::size_t>(target_index_count, 3);

            cluster_group simplified = merge_cluster_bounds(clusters, group);

            simplify_result simplified_result = simplify(merged, locks, target_index_count);

            simplified.max_parent_lod_error =
                std::max(simplified.min_lod_error, simplified_result.error);
            simplified.min_lod_error = simplified.max_parent_lod_error;

            std::uint32_t refined = output_group(clusters, group, simplified, lod);

            std::vector<meshopt_cluster> split = clusterize(
                &m_positions[0].x,
                sizeof(vec3f),
                simplified_result.indexes.data(),
                simplified_result.indexes.size());

            for (meshopt_cluster& cluster : split)
            {
                compute_cluster_bounds(cluster, simplified.max_parent_lod_error);
                cluster.lod_bounds = simplified.lod_bounds;
                cluster.refined = refined;

                clusters.push_back(std::move(cluster));
                pending.push_back(static_cast<int>(clusters.size() - 1));
            }
        }

        ++lod;
    }

    if (!pending.empty())
    {
        assert(pending.size() == 1);

        cluster_group simplified = merge_cluster_bounds(clusters, pending);
        simplified.max_parent_lod_error = std::numeric_limits<float>::infinity();
        output_group(clusters, pending, simplified, lod);
    }

    std::uint32_t first_lod = 0;
    std::vector<std::uint32_t> bvh_indexes;
    while (first_lod < m_groups.size())
    {
        std::uint32_t lod_index = m_groups[first_lod].lod;
        std::uint32_t lod_end = first_lod;
        while (lod_end < m_groups.size() && m_groups[lod_end].lod == lod_index)
        {
            ++lod_end;
        }

        std::vector<std::uint32_t> group_indexes(lod_end - first_lod);
        std::iota(group_indexes.begin(), group_indexes.end(), first_lod);
        bvh_indexes.push_back(build_bvh(group_indexes, false));

        first_lod = lod_end;
    }

    if (bvh_indexes.size() > 1)
    {
        build_bvh(bvh_indexes, true);
    }
    else if (!bvh_indexes.empty())
    {
        auto& root = m_cluster_nodes.emplace_back();
        root.is_leaf = false;
        root.children.push_back(bvh_indexes[0]);
    }

    calculate_bvh_error();
    calculate_bvh_depth();
}

std::vector<cluster_builder_meshopt::meshopt_cluster> cluster_builder_meshopt::clusterize(
    const float* positions,
    std::size_t position_stride,
    const std::uint32_t* indexes,
    size_t index_count)
{
    std::size_t max_meshlets =
        meshopt_buildMeshletsBound(index_count, m_options.max_vertices, m_options.min_triangles);

    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    std::vector<std::uint32_t> meshlet_vertices(index_count);
    std::vector<std::uint8_t> meshlet_triangles(index_count);

    if (m_options.cluster_spatial)
    {
        meshlets.resize(meshopt_buildMeshletsSpatial(
            meshlets.data(),
            meshlet_vertices.data(),
            meshlet_triangles.data(),
            indexes,
            index_count,
            positions,
            m_positions.size(),
            position_stride,
            m_options.max_vertices,
            m_options.min_triangles,
            m_options.max_triangles,
            m_options.cluster_fill_weight));
    }
    else
    {
        meshlets.resize(meshopt_buildMeshletsFlex(
            meshlets.data(),
            meshlet_vertices.data(),
            meshlet_triangles.data(),
            indexes,
            index_count,
            positions,
            m_positions.size(),
            position_stride,
            m_options.max_vertices,
            m_options.min_triangles,
            m_options.max_triangles,
            0.0f,
            m_options.cluster_split_factor));
    }

    std::vector<meshopt_cluster> clusters(meshlets.size());

    for (std::size_t i = 0; i < meshlets.size(); ++i)
    {
        const auto& meshlet = meshlets[i];

        if (m_options.optimize_clusters)
        {
            meshopt_optimizeMeshletLevel(
                &meshlet_vertices[meshlet.vertex_offset],
                meshlet.vertex_count,
                &meshlet_triangles[meshlet.triangle_offset],
                meshlet.triangle_count,
                m_options.optimize_clusters_level);
        }

        clusters[i].vertex_count = meshlet.vertex_count;
        clusters[i].indices.resize(static_cast<std::size_t>(meshlet.triangle_count) * 3);

        for (std::size_t j = 0; j < static_cast<std::size_t>(meshlet.triangle_count) * 3; ++j)
        {
            clusters[i].indices[j] = meshlet_vertices
                [meshlet.vertex_offset + meshlet_triangles[meshlet.triangle_offset + j]];
        }

        clusters[i].refined = -1;
    }

    return clusters;
}

void cluster_builder_meshopt::compute_cluster_bounds(meshopt_cluster& cluster, float lod_error)
{
    meshopt_Bounds bounds = meshopt_computeClusterBounds(
        cluster.indices.data(),
        cluster.indices.size(),
        &m_positions[0].x,
        m_positions.size(),
        sizeof(vec3f));

    cluster.bounding_sphere = {
        .center = {.x = bounds.center[0], .y = bounds.center[1], .z = bounds.center[2]},
        .radius = bounds.radius,
    };

    cluster.bounding_box = {};
    for (std::uint32_t index : cluster.indices)
    {
        box::expand(cluster.bounding_box, m_positions[index]);
    }

    cluster.lod_bounds = cluster.bounding_sphere;
    cluster.lod_error = lod_error;
}

cluster_builder_meshopt::cluster_group cluster_builder_meshopt::merge_cluster_bounds(
    const std::vector<meshopt_cluster>& clusters,
    std::span<const int> group)
{
    cluster_group result = {
        .min_lod_error = 0.0f,
        .max_parent_lod_error = 0.0f,
    };

    std::vector<sphere3f> bounding_spheres;
    std::vector<sphere3f> lod_bounds;
    bounding_spheres.reserve(group.size());
    lod_bounds.reserve(group.size());

    for (int cluster_index : group)
    {
        const meshopt_cluster& cluster = clusters[cluster_index];

        box::expand(result.bounding_box, cluster.bounding_box);
        bounding_spheres.push_back(cluster.bounding_sphere);
        lod_bounds.push_back(cluster.lod_bounds);

        result.min_lod_error = std::max(result.min_lod_error, cluster.lod_error);
    }

    result.bounding_sphere = sphere::create(bounding_spheres);
    result.lod_bounds = sphere::create(lod_bounds);

    return result;
}

std::vector<std::vector<int>> cluster_builder_meshopt::partition(
    const std::vector<meshopt_cluster>& clusters,
    const std::vector<int>& pending,
    const std::vector<std::uint32_t>& remap)
{
    if (pending.size() <= m_options.partition_size)
    {
        return {pending};
    }

    std::size_t total_index_count = 0;
    for (int cluster_index : pending)
    {
        total_index_count += clusters[cluster_index].indices.size();
    }

    std::vector<std::uint32_t> cluster_indices;
    std::vector<std::uint32_t> cluster_counts(pending.size());
    cluster_indices.reserve(total_index_count);

    for (std::size_t i = 0; i < pending.size(); ++i)
    {
        const meshopt_cluster& cluster = clusters[pending[i]];
        cluster_counts[i] = static_cast<std::uint32_t>(cluster.indices.size());

        for (std::uint32_t index : cluster.indices)
        {
            cluster_indices.push_back(remap[index]);
        }
    }

    std::vector<std::uint32_t> cluster_part(pending.size());
    std::size_t partition_count = meshopt_partitionClusters(
        cluster_part.data(),
        cluster_indices.data(),
        cluster_indices.size(),
        cluster_counts.data(),
        cluster_counts.size(),
        m_options.partition_spatial ? &m_positions[0].x : nullptr,
        remap.size(),
        sizeof(vec3f),
        m_options.partition_size);

    std::vector<std::vector<int>> partitions(partition_count);
    for (std::size_t i = 0; i < partition_count; ++i)
    {
        partitions[i].reserve(m_options.partition_size + (m_options.partition_size / 3));
    }

    std::vector<std::uint32_t> partition_remap;
    if (m_options.partition_sort)
    {
        std::vector<float> partition_points(partition_count * 3);
        for (std::size_t i = 0; i < pending.size(); ++i)
        {
            const vec3f center = clusters[pending[i]].bounding_sphere.center;
            float* point = &partition_points[cluster_part[i] * 3ull];
            point[0] = center.x;
            point[1] = center.y;
            point[2] = center.z;
        }

        partition_remap.resize(partition_count);
        meshopt_spatialSortRemap(
            partition_remap.data(),
            partition_points.data(),
            partition_count,
            sizeof(float) * 3);
    }

    for (std::size_t i = 0; i < pending.size(); ++i)
    {
        std::uint32_t partition_index =
            partition_remap.empty() ? cluster_part[i] : partition_remap[cluster_part[i]];
        partitions[partition_index].push_back(pending[i]);
    }

    return partitions;
}

void cluster_builder_meshopt::lock_boundaries(
    std::vector<std::uint8_t>& locks,
    const std::vector<std::vector<int>>& groups,
    const std::vector<meshopt_cluster>& clusters,
    const std::vector<std::uint32_t>& remap)
{
    for (std::uint8_t& lock : locks)
    {
        lock &= ~((1u << 0) | (1u << 7));
    }

    for (const auto& group : groups)
    {
        for (int cluster_index : group)
        {
            for (std::uint32_t index : clusters[cluster_index].indices)
            {
                std::uint32_t r = remap[index];
                locks[r] |= locks[r] >> 7;
            }
        }

        for (int cluster_index : group)
        {
            for (std::uint32_t index : clusters[cluster_index].indices)
            {
                std::uint32_t r = remap[index];
                locks[r] |= 1u << 7;
            }
        }
    }

    for (std::size_t i = 0; i < locks.size(); ++i)
    {
        std::uint32_t r = remap[i];
        locks[i] = (locks[r] & 1u) | (locks[i] & meshopt_SimplifyVertex_Protect);
    }
}

cluster_builder_meshopt::simplify_result cluster_builder_meshopt::simplify(
    const std::vector<std::uint32_t>& indexes,
    const std::vector<std::uint8_t>& locks,
    std::size_t target_index_count)
{
    if (target_index_count > indexes.size())
    {
        return {.indexes = indexes, .error = 0.0f};
    }

    std::size_t attribute_stride = get_attribute_count();

    std::vector<float> attribute_weights;
    if (!m_normals.empty())
    {
        attribute_weights.insert(attribute_weights.end(), {0.5f, 0.5f, 0.5f});
    }

    std::unordered_map<std::uint32_t, std::uint32_t> vertex_remap;
    std::vector<vec3f> positions;
    std::vector<float> attributes;
    std::vector<std::uint8_t> local_locks;
    std::vector<std::uint32_t> lod;

    vertex_remap.reserve(indexes.size());
    positions.reserve(indexes.size());
    attributes.reserve(indexes.size() * attribute_stride);
    local_locks.reserve(indexes.size());
    lod.reserve(indexes.size());

    for (std::uint32_t index : indexes)
    {
        auto [iter, inserted] =
            vertex_remap.emplace(index, static_cast<std::uint32_t>(positions.size()));
        if (inserted)
        {
            positions.push_back(m_positions[index]);

            if (!m_normals.empty())
            {
                const vec3f& normal = m_normals[index];
                attributes.push_back(normal.x);
                attributes.push_back(normal.y);
                attributes.push_back(normal.z);
            }

            if (!m_tangents.empty())
            {
                const vec4f& tangent = m_tangents[index];
                attributes.push_back(tangent.x);
                attributes.push_back(tangent.y);
                attributes.push_back(tangent.z);
                attributes.push_back(tangent.w);
            }

            if (!m_texcoords.empty())
            {
                const vec2f& texcoord = m_texcoords[index];
                attributes.push_back(texcoord.x);
                attributes.push_back(texcoord.y);
            }

            local_locks.push_back(locks[index]);
        }

        lod.push_back(iter->second);
    }

    float error = 0.0f;
    unsigned int simplify_options = meshopt_SimplifyErrorAbsolute | meshopt_SimplifyPermissive;

    lod.resize(meshopt_simplifyWithUpdate(
        lod.data(),
        lod.size(),
        &positions[0].x,
        positions.size(),
        sizeof(vec3f),
        attributes.data(),
        sizeof(float) * attribute_stride,
        attribute_weights.data(),
        attribute_weights.size(),
        local_locks.data(),
        target_index_count,
        FLT_MAX,
        simplify_options,
        &error));

    if (lod.size() >= indexes.size() && target_index_count < indexes.size())
    {
        std::vector<sloppy_vertex> subset(indexes.size());
        std::vector<std::uint8_t> subset_locks(indexes.size());

        lod.resize(indexes.size());
        for (std::size_t i = 0; i < indexes.size(); ++i)
        {
            std::uint32_t index = indexes[i];
            const vec3f& position = m_positions[index];

            subset[i] = {
                .x = position.x,
                .y = position.y,
                .z = position.z,
                .id = index,
            };
            subset_locks[i] = locks[index];
            lod[i] = static_cast<std::uint32_t>(i);
        }

        lod.resize(meshopt_simplifySloppy(
            lod.data(),
            lod.data(),
            lod.size(),
            &subset[0].x,
            subset.size(),
            sizeof(sloppy_vertex),
            subset_locks.data(),
            target_index_count,
            FLT_MAX,
            &error));

        if (lod.size() >= indexes.size())
        {
            lod.resize(indexes.size());
            std::iota(lod.begin(), lod.end(), 0);

            lod.resize(meshopt_simplifySloppy(
                lod.data(),
                lod.data(),
                lod.size(),
                &subset[0].x,
                subset.size(),
                sizeof(sloppy_vertex),
                nullptr,
                target_index_count,
                FLT_MAX,
                &error));
        }

        error *= meshopt_simplifyScale(&subset[0].x, subset.size(), sizeof(sloppy_vertex));
        error *= SIMPLIFY_ERROR_FACTOR_SLOPPY;

        for (std::uint32_t& index : lod)
        {
            index = subset[index].id;
        }

        if (lod.size() < indexes.size())
        {
            return {.indexes = std::move(lod), .error = error};
        }

        std::vector<std::uint32_t> forced(
            indexes.begin(),
            indexes.begin() + static_cast<std::uint32_t>(target_index_count));
        return {.indexes = std::move(forced), .error = error};
    }

    std::vector<std::uint32_t> compact_remap(
        positions.size(),
        std::numeric_limits<std::uint32_t>::max());
    std::vector<std::uint32_t> result;
    result.reserve(lod.size());

    for (std::uint32_t index : lod)
    {
        std::uint32_t& compact_index = compact_remap[index];
        if (compact_index == std::numeric_limits<std::uint32_t>::max())
        {
            compact_index = static_cast<std::uint32_t>(m_positions.size());
            m_positions.push_back(positions[index]);

            const float* attribute = attributes.data() + (index * attribute_stride);
            std::uint32_t attribute_offset = 0;

            if (!m_normals.empty())
            {
                vec3f normal = {
                    attribute[attribute_offset + 0],
                    attribute[attribute_offset + 1],
                    attribute[attribute_offset + 2],
                };
                normal = vector::length_sq(normal) > 1e-8f ? vector::normalize(normal) :
                                                             vec3f{0.0f, 1.0f, 0.0f};
                m_normals.push_back(normal);

                attribute_offset += 3;
            }

            if (!m_tangents.empty())
            {
                vec3f tangent = {
                    attribute[attribute_offset + 0],
                    attribute[attribute_offset + 1],
                    attribute[attribute_offset + 2],
                };
                tangent = vector::length_sq(tangent) > 1e-8f ? vector::normalize(tangent) :
                                                               vec3f{1.0f, 0.0f, 0.0f};
                m_tangents.push_back({
                    .x = tangent.x,
                    .y = tangent.y,
                    .z = tangent.z,
                    .w = attribute[attribute_offset + 3] < 0.0f ? -1.0f : 1.0f,
                });

                attribute_offset += 4;
            }

            if (!m_texcoords.empty())
            {
                m_texcoords.push_back({
                    .x = attribute[attribute_offset + 0],
                    .y = attribute[attribute_offset + 1],
                });

                attribute_offset += 2;
            }
        }

        result.push_back(compact_index);
    }

    return {.indexes = std::move(result), .error = error};
}

std::uint32_t cluster_builder_meshopt::output_group(
    const std::vector<meshopt_cluster>& clusters,
    std::span<const int> group,
    const cluster_group& simplified,
    std::uint32_t lod)
{
    auto group_index = static_cast<std::uint32_t>(m_groups.size());
    auto cluster_offset = static_cast<std::uint32_t>(m_clusters.size());

    cluster_group output = simplified;
    output.min_lod_error = std::numeric_limits<float>::infinity();
    output.cluster_offset = cluster_offset;
    output.cluster_count = static_cast<std::uint32_t>(group.size());
    output.lod = lod;

    for (int source_index : group)
    {
        const meshopt_cluster& source_cluster = clusters[source_index];

        output.min_lod_error = std::min(output.min_lod_error, source_cluster.lod_error);

        cluster output_cluster = {
            .index_offset = static_cast<std::uint32_t>(m_indexes.size()),
            .index_count = static_cast<std::uint32_t>(source_cluster.indices.size()),
            .bounding_box = source_cluster.bounding_box,
            .bounding_sphere = source_cluster.bounding_sphere,
            .lod_bounds = source_cluster.lod_bounds,
            .lod_error = source_cluster.lod_error,
            .group_index = group_index,
            .child_group_index = source_cluster.refined,
        };

        m_indexes.insert(
            m_indexes.end(),
            source_cluster.indices.begin(),
            source_cluster.indices.end());
        m_clusters.push_back(output_cluster);
    }

    m_groups.push_back(output);
    return group_index;
}

std::uint32_t cluster_builder_meshopt::build_bvh(std::span<std::uint32_t> indexes, bool root)
{
    auto child_count = static_cast<std::uint32_t>(indexes.size());

    if (child_count == 1)
    {
        if (root)
        {
            return indexes[0];
        }

        auto node_index = static_cast<std::uint32_t>(m_cluster_nodes.size());
        auto& node = m_cluster_nodes.emplace_back();
        node.is_leaf = true;
        node.children.push_back(indexes[0]);
        return node_index;
    }

    if (child_count <= MAX_BVH_CHILD_COUNT)
    {
        std::vector<std::uint32_t> children;
        children.reserve(child_count);
        for (std::uint32_t i = 0; i < child_count; ++i)
        {
            children.push_back(build_bvh(indexes.subspan(i, 1), root));
        }

        auto node_index = static_cast<std::uint32_t>(m_cluster_nodes.size());
        auto& node = m_cluster_nodes.emplace_back();
        node.children = children;
        return node_index;
    }

    std::uint32_t large_child_count = MAX_BVH_CHILD_COUNT;
    while (large_child_count * MAX_BVH_CHILD_COUNT < child_count)
    {
        large_child_count *= MAX_BVH_CHILD_COUNT;
    }
    std::uint32_t small_child_count = large_child_count / MAX_BVH_CHILD_COUNT;
    std::uint32_t excess_child_count = child_count - (small_child_count * MAX_BVH_CHILD_COUNT);

    std::array<std::uint32_t, MAX_BVH_CHILD_COUNT> child_sizes;
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
                    split0 += child_sizes[(j * range_size) + k];
                    split1 += child_sizes[(j * range_size) + k + (range_size / 2)];
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

    auto node_index = static_cast<std::uint32_t>(m_cluster_nodes.size());
    auto& node = m_cluster_nodes.emplace_back();
    node.children = children;
    return node_index;
}

void cluster_builder_meshopt::sort_groups(
    std::span<std::uint32_t> group_indexes,
    std::uint32_t split)
{
    auto get_surface_area = [&](std::uint32_t begin, std::uint32_t end)
    {
        box3f bounding_box;
        for (std::uint32_t i = begin; i < end; ++i)
        {
            box::expand(bounding_box, m_groups[group_indexes[i]].bounding_box);
        }

        vec3f extent = box::get_extent(bounding_box);
        return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
    };

    float min_surface_area = std::numeric_limits<float>::infinity();
    std::uint32_t best_axis = 0;
    for (std::uint32_t axis = 0; axis < 3; ++axis)
    {
        std::ranges::sort(
            group_indexes,
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

    std::ranges::sort(
        group_indexes,
        [&](std::uint32_t a, std::uint32_t b) -> bool
        {
            vec3f a_center = box::get_center(m_groups[a].bounding_box);
            vec3f b_center = box::get_center(m_groups[b].bounding_box);
            return a_center[best_axis] < b_center[best_axis];
        });
}

void cluster_builder_meshopt::calculate_bvh_error()
{
    for (auto iter = m_cluster_nodes.begin(); iter != m_cluster_nodes.end(); ++iter)
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
                const auto& child = m_cluster_nodes[node_index];

                box::expand(iter->bounding_box, child.bounding_box);
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

void cluster_builder_meshopt::calculate_bvh_depth()
{
    for (auto iter = m_cluster_nodes.rbegin(); iter != m_cluster_nodes.rend(); ++iter)
    {
        if (!iter->is_leaf)
        {
            for (std::uint32_t child : iter->children)
            {
                m_cluster_nodes[child].depth = iter->depth + 1;
            }
        }
    }
}
} // namespace violet
