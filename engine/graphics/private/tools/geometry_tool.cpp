#include "graphics/tools/geometry_tool.hpp"
#include "algorithm/hash.hpp"
#include "math/vector.hpp"
#include "mikktspace.h"
#include "tools/mesh_simplifier/mesh_simplifier.hpp"
#include <unordered_map>

namespace violet
{
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
} // namespace violet