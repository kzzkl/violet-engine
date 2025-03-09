#pragma once

#include "math/types.hpp"
#include <span>
#include <vector>

namespace violet
{
class geometry_tool
{
public:
    static std::vector<vec4f> generate_tangents(
        std::span<const vec3f> positions,
        std::span<const vec3f> normals,
        std::span<const vec2f> texcoords,
        std::span<const std::uint32_t> indexes);

    static std::vector<vec3f> generate_smooth_normals(
        std::span<const vec3f> positions,
        std::span<const vec3f> normals,
        std::span<const vec4f> tangents,
        std::span<const std::uint32_t> indexes);
};
} // namespace violet