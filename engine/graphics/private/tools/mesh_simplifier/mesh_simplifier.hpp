#pragma once

#include "algorithm/hash.hpp"
#include "math/types.hpp"
#include "tools/mesh_simplifier/collapse_heap.hpp"
#include "tools/mesh_simplifier/quadric.hpp"
#include <span>
#include <unordered_map>

namespace violet
{
class mesh_simplifier
{
public:
    void set_positions(std::span<const vec3f> positions);
    void set_indexes(std::span<const std::uint32_t> indexes);

    void lock_position(const vec3f& position);

    float simplify(
        std::uint32_t target_triangle_count,
        std::vector<vec3f>& new_positions,
        std::vector<std::uint32_t>& new_indexes);

private:
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

    struct edge
    {
        vec3f p0;
        vec3f p1;

        vec3f merge_position;

        bool removed;

        bool operator==(const edge& other) const noexcept
        {
            return p0 == other.p0 && p1 == other.p1;
        }
    };

    enum corner_flag : std::uint8_t
    {
        CORNER_LOCKED = 1 << 0,
        CORNER_REMOVED = 1 << 1,
    };
    using corner_flags = std::uint8_t;

    float evaluate_edge(std::uint32_t edge_index);
    void collapse_edge(std::uint32_t edge_index);

    void compact();

    bool add_edge(edge& edge, std::uint32_t index);
    void remove_edge(std::uint32_t index);

    bool get_adjacent_triangles(
        const vec3f& position,
        std::vector<std::uint32_t>& adjacent_triangles);

    bool is_triangle_flip(const vec3f& old_position, const vec3f& new_position, std::uint32_t index)
        const;

    std::vector<vec3f> m_positions;
    std::vector<std::uint32_t> m_indexes;

    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_vertex_map;
    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_corner_map;
    std::vector<corner_flags> m_corner_flags;

    std::vector<edge> m_edges;
    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_edge_map0;
    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_edge_map1;

    std::vector<quadric> m_triangle_quadrics;
    std::uint32_t m_triangle_count{0};

    collapse_heap m_heap;
};
} // namespace violet