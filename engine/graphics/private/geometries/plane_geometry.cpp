#include "graphics/geometries/plane_geometry.hpp"
#include "graphics/tools/geometry_tool.hpp"

namespace violet
{
plane_geometry::plane_geometry(
    float width,
    float height,
    std::size_t width_segments,
    std::size_t height_segments)
{
    // https://github.com/mrdoob/three.js/blob/dev/src/geometries/PlaneGeometry.js

    const float width_half = width / 2;
    const float height_half = height / 2;

    const std::size_t grid_x = width_segments;
    const std::size_t grid_y = height_segments;

    const int grid_x1 = static_cast<int>(grid_x + 1);
    const int grid_y1 = static_cast<int>(grid_y + 1);

    const float segment_width = width / static_cast<float>(grid_x);
    const float segment_height = height / static_cast<float>(grid_y);

    std::vector<vec3f> positions;
    std::vector<vec3f> normals;
    std::vector<vec2f> texcoords;
    std::vector<std::uint32_t> indexes;

    for (int i = 0; i < grid_y1; ++i)
    {
        const float y = static_cast<float>(i) * segment_height - height_half;

        for (int j = 0; j < grid_x1; ++j)
        {
            const float x = static_cast<float>(j) * segment_width - width_half;

            positions.push_back({x, -y, 0});
            normals.push_back({0, 0, 1});

            vec2f texcoord;
            texcoord.x = static_cast<float>(j) / static_cast<float>(grid_x);
            texcoord.y = 1.0f - (static_cast<float>(i) / static_cast<float>(grid_y));
            texcoords.push_back(texcoord);
        }
    }

    for (int iy = 0; iy < grid_y; iy++)
    {
        for (int ix = 0; ix < grid_x; ix++)
        {
            const std::uint32_t a = ix + grid_x1 * iy;
            const std::uint32_t b = ix + grid_x1 * (iy + 1);
            const std::uint32_t c = (ix + 1) + grid_x1 * (iy + 1);
            const std::uint32_t d = (ix + 1) + grid_x1 * iy;

            indexes.push_back(a);
            indexes.push_back(d);
            indexes.push_back(b);

            indexes.push_back(b);
            indexes.push_back(d);
            indexes.push_back(c);
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