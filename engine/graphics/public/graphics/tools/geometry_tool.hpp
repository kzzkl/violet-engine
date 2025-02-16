#pragma once

#include "math/types.hpp"
#include <vector>

namespace violet
{
class geometry_tool
{
public:
    static std::vector<vec3f> generate_tangents(
        const std::vector<vec3f>& positions,
        const std::vector<vec3f>& normals,
        const std::vector<vec2f>& texcoords,
        const std::vector<std::uint32_t>& indexes);

    static std::vector<vec3f> generate_smooth_normals(
        const std::vector<vec3f>& positions,
        const std::vector<vec3f>& normals,
        const std::vector<vec3f>& tangents,
        const std::vector<std::uint32_t>& indexes);
};
} // namespace violet