#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "math/vector.hpp"

namespace violet
{
sphere_geometry::sphere_geometry(
    float radius,
    std::size_t width_segments,
    std::size_t height_segments,
    float phi_start,
    float phi_length,
    float theta_start,
    float theta_length)
{
    // https://github.com/mrdoob/three.js/blob/master/src/geometries/SphereGeometry.js

    std::vector<vec3f> positions;
    std::vector<vec3f> normals;
    std::vector<vec2f> texcoords;
    std::vector<std::uint32_t> indexes;

    width_segments = std::max(3ull, width_segments);
    height_segments = std::max(2ull, height_segments);

    float theta_end = std::min(theta_start + theta_length, math::PI);

    std::uint32_t index = 0;
    std::vector<std::vector<std::uint32_t>> grid;

    // generate vertices, normals and uvs
    for (std::size_t iy = 0; iy <= height_segments; ++iy)
    {
        std::vector<std::uint32_t> vertices_row;

        float v = static_cast<float>(iy) / static_cast<float>(height_segments);

        // special case for the poles
        float u_offset = 0.0f;

        if (iy == 0 && theta_start == 0.0f)
        {
            u_offset = 0.5f / static_cast<float>(width_segments);
        }
        else if (iy == height_segments && theta_end == math::PI)
        {
            u_offset = -0.5f / static_cast<float>(width_segments);
        }

        for (std::size_t ix = 0; ix <= width_segments; ++ix)
        {
            float u = static_cast<float>(ix) / static_cast<float>(width_segments);

            // vertex
            vec3f vertex;
            vertex.x = -radius * std::cos(phi_start + u * phi_length) *
                       std::sin(theta_start + v * theta_length);
            vertex.y = radius * std::cos(theta_start + v * theta_length);
            vertex.z = radius * std::sin(phi_start + u * phi_length) *
                       std::sin(theta_start + v * theta_length);

            positions.push_back(vertex);

            // normal
            vec3f normal = vector::normalize(vertex);
            normals.push_back(normal);

            // uv
            texcoords.push_back({u + u_offset, 1 - v});

            vertices_row.push_back(index++);
        }

        grid.push_back(vertices_row);
    }

    // indexes
    for (std::size_t iy = 0; iy < height_segments; ++iy)
    {
        for (std::size_t ix = 0; ix < width_segments; ++ix)
        {
            std::uint32_t a = grid[iy][ix + 1];
            std::uint32_t b = grid[iy][ix];
            std::uint32_t c = grid[iy + 1][ix];
            std::uint32_t d = grid[iy + 1][ix + 1];

            if (iy != 0 || theta_start > 0)
            {
                indexes.push_back(a);
                indexes.push_back(d);
                indexes.push_back(b);
            }

            if (iy != height_segments - 1 || theta_end < math::PI)
            {
                indexes.push_back(b);
                indexes.push_back(d);
                indexes.push_back(c);
            }
        }
    }

    std::vector<vec4f> tangents =
        geometry_tool::generate_tangents(positions, normals, texcoords, indexes);

    set_position(positions);
    set_normal(normals);
    set_tangent(tangents);
    set_texcoord(texcoords);
    set_index(indexes);
}
} // namespace violet