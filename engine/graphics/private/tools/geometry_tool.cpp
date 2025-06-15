#include "graphics/tools/geometry_tool.hpp"
#include "algorithm/hash.hpp"
#include "math/vector.hpp"
#include "mikktspace.h"
#include "tools/cluster/cluster_builder.hpp"
#include "tools/mesh_simplifier/mesh_simplifier.hpp"
#include <fstream>
#include <queue>
#include <unordered_map>

namespace violet
{
bool geometry_tool::cluster_output::load(std::string_view path)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
    {
        return false;
    }

    std::uint32_t vertex_count = 0;
    std::uint32_t index_count = 0;
    std::uint32_t submesh_count = 0;

    fin.read(reinterpret_cast<char*>(&vertex_count), sizeof(std::uint32_t));
    fin.read(reinterpret_cast<char*>(&index_count), sizeof(std::uint32_t));
    fin.read(reinterpret_cast<char*>(&submesh_count), sizeof(std::uint32_t));

    positions.resize(vertex_count);
    indexes.resize(index_count);
    submeshes.resize(submesh_count);

    fin.read(
        reinterpret_cast<char*>(positions.data()),
        static_cast<std::streamsize>(vertex_count * sizeof(vec3f)));
    fin.read(
        reinterpret_cast<char*>(indexes.data()),
        static_cast<std::streamsize>(index_count * sizeof(std::uint32_t)));

    for (std::uint32_t i = 0; i < submesh_count; ++i)
    {
        std::uint32_t cluster_count = 0;
        std::uint32_t cluster_node_count = 0;

        fin.read(reinterpret_cast<char*>(&cluster_count), sizeof(std::uint32_t));
        fin.read(reinterpret_cast<char*>(&cluster_node_count), sizeof(std::uint32_t));

        submeshes[i].clusters.resize(cluster_count);
        submeshes[i].cluster_nodes.resize(cluster_node_count);

        for (auto& cluster : submeshes[i].clusters)
        {
            fin.read(reinterpret_cast<char*>(&cluster.index_offset), sizeof(std::uint32_t));
            fin.read(reinterpret_cast<char*>(&cluster.index_count), sizeof(std::uint32_t));
            fin.read(reinterpret_cast<char*>(&cluster.bounding_box), sizeof(box3f));
            fin.read(reinterpret_cast<char*>(&cluster.bounding_sphere), sizeof(sphere3f));
            fin.read(reinterpret_cast<char*>(&cluster.lod_bounds), sizeof(sphere3f));
            fin.read(reinterpret_cast<char*>(&cluster.lod_error), sizeof(float));
            fin.read(reinterpret_cast<char*>(&cluster.parent_lod_bounds), sizeof(sphere3f));
            fin.read(reinterpret_cast<char*>(&cluster.parent_lod_error), sizeof(float));
            fin.read(reinterpret_cast<char*>(&cluster.lod), sizeof(std::uint32_t));
        }

        for (auto& cluster_node : submeshes[i].cluster_nodes)
        {
            fin.read(reinterpret_cast<char*>(&cluster_node.bounding_sphere), sizeof(sphere3f));
            fin.read(reinterpret_cast<char*>(&cluster_node.lod_bounds), sizeof(sphere3f));
            fin.read(reinterpret_cast<char*>(&cluster_node.min_lod_error), sizeof(float));
            fin.read(reinterpret_cast<char*>(&cluster_node.max_parent_lod_error), sizeof(float));
            fin.read(reinterpret_cast<char*>(&cluster_node.is_leaf), sizeof(bool));
            fin.read(reinterpret_cast<char*>(&cluster_node.depth), sizeof(std::uint32_t));
            fin.read(reinterpret_cast<char*>(&cluster_node.child_offset), sizeof(std::uint32_t));
            fin.read(reinterpret_cast<char*>(&cluster_node.child_count), sizeof(std::uint32_t));
        }
    }

    return true;
}

bool geometry_tool::cluster_output::save(std::string_view path) const
{
    std::ofstream fout(path.data(), std::ios::binary);
    if (!fout.is_open())
    {
        return false;
    }

    auto vertex_count = static_cast<std::uint32_t>(positions.size());
    auto index_count = static_cast<std::uint32_t>(indexes.size());
    auto submesh_count = static_cast<std::uint32_t>(submeshes.size());

    fout.write(reinterpret_cast<const char*>(&vertex_count), sizeof(std::uint32_t));
    fout.write(reinterpret_cast<const char*>(&index_count), sizeof(std::uint32_t));
    fout.write(reinterpret_cast<const char*>(&submesh_count), sizeof(std::uint32_t));

    fout.write(
        reinterpret_cast<const char*>(positions.data()),
        static_cast<std::streamsize>(vertex_count * sizeof(vec3f)));
    fout.write(
        reinterpret_cast<const char*>(indexes.data()),
        static_cast<std::streamsize>(index_count * sizeof(std::uint32_t)));

    for (const auto& submesh : submeshes)
    {
        auto cluster_count = static_cast<std::uint32_t>(submesh.clusters.size());
        auto cluster_node_count = static_cast<std::uint32_t>(submesh.cluster_nodes.size());

        fout.write(reinterpret_cast<const char*>(&cluster_count), sizeof(std::uint32_t));
        fout.write(reinterpret_cast<const char*>(&cluster_node_count), sizeof(std::uint32_t));

        for (const auto& cluster : submesh.clusters)
        {
            fout.write(reinterpret_cast<const char*>(&cluster.index_offset), sizeof(std::uint32_t));
            fout.write(reinterpret_cast<const char*>(&cluster.index_count), sizeof(std::uint32_t));
            fout.write(reinterpret_cast<const char*>(&cluster.bounding_box), sizeof(box3f));
            fout.write(reinterpret_cast<const char*>(&cluster.bounding_sphere), sizeof(sphere3f));
            fout.write(reinterpret_cast<const char*>(&cluster.lod_bounds), sizeof(sphere3f));
            fout.write(reinterpret_cast<const char*>(&cluster.lod_error), sizeof(float));
            fout.write(reinterpret_cast<const char*>(&cluster.parent_lod_bounds), sizeof(sphere3f));
            fout.write(reinterpret_cast<const char*>(&cluster.parent_lod_error), sizeof(float));
            fout.write(reinterpret_cast<const char*>(&cluster.lod), sizeof(std::uint32_t));
        }

        for (const auto& cluster_node : submesh.cluster_nodes)
        {
            fout.write(
                reinterpret_cast<const char*>(&cluster_node.bounding_sphere),
                sizeof(sphere3f));
            fout.write(reinterpret_cast<const char*>(&cluster_node.lod_bounds), sizeof(sphere3f));
            fout.write(reinterpret_cast<const char*>(&cluster_node.min_lod_error), sizeof(float));
            fout.write(
                reinterpret_cast<const char*>(&cluster_node.max_parent_lod_error),
                sizeof(float));
            fout.write(reinterpret_cast<const char*>(&cluster_node.is_leaf), sizeof(bool));
            fout.write(reinterpret_cast<const char*>(&cluster_node.depth), sizeof(std::uint32_t));
            fout.write(
                reinterpret_cast<const char*>(&cluster_node.child_offset),
                sizeof(std::uint32_t));
            fout.write(
                reinterpret_cast<const char*>(&cluster_node.child_count),
                sizeof(std::uint32_t));
        }
    }

    return false;
}

std::vector<vec4f> geometry_tool::generate_tangents(
    std::span<const vec3f> positions,
    std::span<const vec3f> normals,
    std::span<const vec2f> texcoords,
    std::span<const std::uint32_t> indexes)
{
    struct tangent_context : public SMikkTSpaceContext
    {
        std::span<const vec3f> positions;
        std::span<const vec3f> normals;
        std::span<const vec2f> texcoords;
        std::span<const std::uint32_t> indexes;

        std::vector<vec4f>& tangents;
    };

    SMikkTSpaceInterface interface = {};
    interface.m_getNumFaces = [](const SMikkTSpaceContext* context) -> int
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        return static_cast<int>(ctx->indexes.size() / 3);
    };
    interface.m_getNumVerticesOfFace = [](const SMikkTSpaceContext* context, const int face) -> int
    {
        return 3;
    };
    interface.m_getPosition =
        [](const SMikkTSpaceContext* context, float* out, const int face, const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        const vec3f& position = ctx->positions[ctx->indexes[face * 3 + vert]];
        out[0] = position.x;
        out[1] = position.y;
        out[2] = position.z;
    };
    interface.m_getNormal =
        [](const SMikkTSpaceContext* context, float* out, const int face, const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        const vec3f& normal = ctx->normals[ctx->indexes[face * 3 + vert]];
        out[0] = normal.x;
        out[1] = normal.y;
        out[2] = normal.z;
    };
    interface.m_getTexCoord =
        [](const SMikkTSpaceContext* context, float* out, const int face, const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        const vec2f& texcoord = ctx->texcoords[ctx->indexes[face * 3 + vert]];
        out[0] = texcoord.x;
        out[1] = texcoord.y;
    };
    interface.m_setTSpaceBasic = [](const SMikkTSpaceContext* context,
                                    const float* in,
                                    const float sign,
                                    const int face,
                                    const int vert)
    {
        const auto* ctx = static_cast<const tangent_context*>(context);
        vec4f& tangent = ctx->tangents[ctx->indexes[face * 3 + vert]];
        tangent.x = in[0];
        tangent.y = in[1];
        tangent.z = in[2];
        tangent.w = sign;
    };

    std::vector<vec4f> result(positions.size());

    tangent_context context = {
        .positions = positions,
        .normals = normals,
        .texcoords = texcoords,
        .indexes = indexes,
        .tangents = result,
    };
    context.m_pInterface = &interface;

    if (!genTangSpaceDefault(&context))
    {
        return {};
    }

    return result;
}

std::vector<vec3f> geometry_tool::generate_smooth_normals(
    std::span<const vec3f> positions,
    std::span<const vec3f> normals,
    std::span<const vec4f> tangents,
    std::span<const std::uint32_t> indexes)
{
    struct vertex_normal
    {
        vec3f normal;
        float weight;
    };

    struct vec3_hash
    {
        std::size_t operator()(const vec3f& v) const noexcept
        {
            return hash::city_hash_64(&v, sizeof(vec3f));
        }
    };

    std::unordered_map<vec3f, std::vector<vertex_normal>, vec3_hash> map;

    auto calculate_weight = [](const vec3f& a, const vec3f& b) -> float
    {
        return std::acos(vector::dot(a, b) / (vector::length(a) * vector::length(b)));
    };

    for (std::size_t i = 0; i < indexes.size(); i += 3)
    {
        vec3f v0 = positions[indexes[i + 0]];
        vec3f v1 = positions[indexes[i + 1]];
        vec3f v2 = positions[indexes[i + 2]];

        vec3f vec0 = v1 - v0;
        vec3f vec1 = v2 - v0;

        vec3f normal = vector::normalize(vector::cross(vec1, vec0));

        map[v0].push_back({normal, calculate_weight(v1 - v0, v2 - v0)});
        map[v1].push_back({normal, calculate_weight(v0 - v1, v2 - v1)});
        map[v2].push_back({normal, calculate_weight(v0 - v2, v1 - v2)});
    }

    std::vector<vec3f> result(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i)
    {
        vec3f smooth_normal = {};
        float total_weight = 0.0f;
        for (auto& [normal, weight] : map[positions[i]])
        {
            smooth_normal = smooth_normal + normal * weight;
            total_weight += weight;
        }

        if (vector::length(smooth_normal) != 0.0f)
        {
            smooth_normal = smooth_normal / total_weight;
            smooth_normal = vector::normalize(smooth_normal);
        }
        else
        {
            smooth_normal = normals[i];
        }

        vec3f tangent = {tangents[i].x, tangents[i].y, tangents[i].z};
        vec3f bitangent = vector::cross(normals[i], tangent) * tangents[i].w;

        result[i].x = vector::dot(smooth_normal, tangent);
        result[i].y = vector::dot(smooth_normal, bitangent);
        result[i].z = vector::dot(smooth_normal, normals[i]);
    }

    return result;
}

geometry_tool::simplify_result geometry_tool::simplify(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes,
    std::uint32_t target_triangle_count,
    std::span<const vec3f> locked_positions)
{
    mesh_simplifier simplifier;
    simplifier.set_positions(positions);
    simplifier.set_indexes(indexes);

    for (const auto& locked_position : locked_positions)
    {
        simplifier.lock_position(locked_position);
    }

    simplifier.simplify(target_triangle_count);

    simplify_result result;
    result.positions = simplifier.get_positions();
    result.indexes = simplifier.get_indexes();

    return result;
}

geometry_tool::simplify_result geometry_tool::simplify_meshopt(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes,
    std::uint32_t target_triangle_count)
{
    mesh_simplifier_meshopt simplifier;
    simplifier.set_positions(positions);
    simplifier.set_indexes(indexes);

    simplifier.simplify(target_triangle_count, false);

    simplify_result result;
    result.positions.assign(positions.begin(), positions.end());
    result.indexes = simplifier.get_indexes();

    return result;
}

geometry_tool::cluster_output geometry_tool::generate_clusters(const cluster_input& input)
{
    cluster_output output;

    for (std::size_t submesh_index = 0; submesh_index < input.submeshes.size(); ++submesh_index)
    {
        const auto& submesh = input.submeshes[submesh_index];

        std::unordered_map<std::uint32_t, std::uint32_t> vertex_remap;

        std::vector<vec3f> positions;
        std::vector<std::uint32_t> indexes;
        indexes.reserve(submesh.index_count);

        for (std::uint32_t i = 0; i < submesh.index_count; ++i)
        {
            std::uint32_t vertex_index =
                input.indexes[submesh.index_offset + i] + submesh.vertex_offset;

            auto iter = vertex_remap.find(vertex_index);
            if (iter == vertex_remap.end())
            {
                positions.push_back(input.positions[vertex_index]);
                vertex_remap[vertex_index] = static_cast<std::uint32_t>(positions.size() - 1);
                vertex_index = static_cast<std::uint32_t>(positions.size() - 1);
            }
            else
            {
                vertex_index = iter->second;
            }

            indexes.push_back(vertex_index);
        }

        cluster_builder builder;
        builder.set_positions(positions);
        builder.set_indexes(indexes);

        builder.build();

        auto vertex_offset = static_cast<std::uint32_t>(output.positions.size());
        auto index_offset = static_cast<std::uint32_t>(output.indexes.size());

        output.positions.insert(
            output.positions.end(),
            builder.get_positions().begin(),
            builder.get_positions().end());
        output.indexes.insert(
            output.indexes.end(),
            builder.get_indexes().begin(),
            builder.get_indexes().end());
        for (std::uint32_t i = index_offset; i < output.indexes.size(); ++i)
        {
            output.indexes[i] += vertex_offset;
        }

        cluster_output::submesh submesh_output;

        const auto& groups = builder.get_groups();
        const auto& clusters = builder.get_clusters();

        for (const auto& group : groups)
        {
            for (std::uint32_t i = 0; i < group.cluster_count; ++i)
            {
                const auto& cluster = clusters[group.cluster_offset + i];

                submesh_output.clusters.push_back({
                    .index_offset = cluster.index_offset + index_offset,
                    .index_count = cluster.index_count,
                    .bounding_box = cluster.bounding_box,
                    .bounding_sphere = cluster.bounding_sphere,
                    .lod_bounds = cluster.lod_bounds,
                    .lod_error = cluster.lod_error,
                    .parent_lod_bounds = group.lod_bounds,
                    .parent_lod_error = group.max_parent_lod_error,
                    .lod = group.lod,
                });
            }
        }

        const auto& cluster_nodes = builder.get_cluster_nodes();
        std::uint32_t bvh_node_count = 0;

        std::queue<std::uint32_t> queue;
        queue.push(static_cast<std::uint32_t>(cluster_nodes.size() - 1));
        while (!queue.empty())
        {
            const auto& cluster_node = cluster_nodes[queue.front()];
            queue.pop();

            submesh_output.cluster_nodes.push_back({
                .bounding_sphere = cluster_node.bounding_sphere,
                .lod_bounds = cluster_node.lod_bounds,
                .min_lod_error = cluster_node.min_lod_error,
                .max_parent_lod_error = cluster_node.max_parent_lod_error,
                .is_leaf = cluster_node.is_leaf,
                .depth = cluster_node.depth,
            });

            ++bvh_node_count;

            if (cluster_node.is_leaf)
            {
                const auto& group = groups[cluster_node.children[0]];
                submesh_output.cluster_nodes.back().child_offset = group.cluster_offset;
                submesh_output.cluster_nodes.back().child_count = group.cluster_count;
            }
            else
            {
                submesh_output.cluster_nodes.back().child_offset =
                    static_cast<std::uint32_t>(queue.size() + bvh_node_count);
                submesh_output.cluster_nodes.back().child_count =
                    static_cast<std::uint32_t>(cluster_node.children.size());

                for (std::uint32_t child : cluster_node.children)
                {
                    queue.push(child);
                }
            }
        }

        output.submeshes.push_back(submesh_output);
    }

    return output;
}
} // namespace violet