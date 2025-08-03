#include "graphics/geometries/box_geometry.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include <array>

namespace violet
{
box_geometry::box_geometry(
    float width,
    float height,
    float depth,
    std::size_t width_segments,
    std::size_t height_segments,
    std::size_t depth_segments)
{
    // https://github.com/mrdoob/three.js/blob/master/src/geometries/BoxGeometry.js

    std::vector<vec3f> positions;
    std::vector<vec3f> normals;
    std::vector<vec2f> texcoords;
    std::vector<std::uint32_t> indexes;

    const std::array<float, 3> extent = {
        width,
        height,
        depth,
    };

    const std::array<std::size_t, 3> segments = {
        width_segments,
        height_segments,
        depth_segments,
    };

    std::uint32_t vertex_offset = 0;
    auto build_plane =
        [&](std::size_t u, std::size_t v, std::size_t w, float udir, float vdir, float wdir)
    {
        const float width = extent[u];
        const float height = extent[v];
        const float depth = extent[w] * wdir;

        const std::size_t grid_x = segments[u];
        const std::size_t grid_y = segments[v];

        const float segment_width = width / static_cast<float>(grid_x);
        const float segment_height = height / static_cast<float>(grid_y);

        const float width_half = width / 2.0f;
        const float height_half = height / 2.0f;
        const float depth_half = depth / 2.0f;

        const int grid_x1 = static_cast<int>(grid_x + 1);
        const int grid_y1 = static_cast<int>(grid_y + 1);

        std::uint32_t vertex_counter = 0;
        for (int i = 0; i < grid_y1; i++)
        {
            const float y = (static_cast<float>(i) * segment_height) - height_half;

            for (int j = 0; j < grid_x1; j++)
            {
                const float x = (static_cast<float>(j) * segment_width) - width_half;

                vec3f position;
                position[u] = x * udir;
                position[v] = y * vdir;
                position[w] = depth_half;
                positions.push_back(position);

                vec3f normal;
                normal[u] = 0;
                normal[v] = 0;
                normal[w] = depth > 0 ? 1.0f : -1.0f;
                normals.push_back(normal);

                vec2f texcoord;
                texcoord.x = static_cast<float>(j) / static_cast<float>(grid_x);
                texcoord.y = 1.0f - (static_cast<float>(i) / static_cast<float>(grid_y));
                texcoords.push_back(texcoord);

                vertex_counter += 1;
            }
        }

        for (int i = 0; i < grid_y; i++)
        {
            for (int j = 0; j < grid_x; j++)
            {
                const std::uint32_t a = vertex_offset + j + (grid_x1 * i);
                const std::uint32_t b = vertex_offset + j + (grid_x1 * (i + 1));
                const std::uint32_t c = vertex_offset + (j + 1) + (grid_x1 * (i + 1));
                const std::uint32_t d = vertex_offset + (j + 1) + (grid_x1 * i);

                indexes.push_back(a);
                indexes.push_back(d);
                indexes.push_back(b);

                indexes.push_back(b);
                indexes.push_back(d);
                indexes.push_back(c);
            }
        }

        vertex_offset += vertex_counter;
    };

    build_plane(2, 1, 0, -1, -1, 1);  // px
    build_plane(2, 1, 0, 1, -1, -1);  // nx
    build_plane(0, 2, 1, 1, 1, 1);    // py
    build_plane(0, 2, 1, 1, -1, -1);  // ny
    build_plane(0, 1, 2, 1, -1, 1);   // pz
    build_plane(0, 1, 2, -1, -1, -1); // nz

    std::vector<vec4f> tangents =
        geometry_tool::generate_tangents(positions, normals, texcoords, indexes);

    set_positions(positions);
    set_normals(normals);
    set_tangents(tangents);
    set_texcoords(texcoords);
    set_indexes(indexes);

    add_submesh(0, 0, static_cast<std::uint32_t>(indexes.size()));
}
} // namespace violet