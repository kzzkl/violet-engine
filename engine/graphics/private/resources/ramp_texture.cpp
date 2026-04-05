#include "graphics/resources/ramp_texture.hpp"
#include "graphics/texture_loader.hpp"
#include "math/vector.hpp"

namespace violet
{
ramp_texture::ramp_texture(const std::vector<point>& points, std::uint32_t width)
{
    assert(points.size() <= width);

    texture_data data = {
        .format = RHI_FORMAT_R8G8B8A8_UNORM,
        .extent = {.width = width, .height = 1, .depth = 1},
        .layer_count = 1,
        .level_count = 1,
    };
    data.pixels.reserve(4ull * width);

    std::size_t prev_index = 0;
    std::size_t next_index = 0;

    for (std::size_t i = 0; i < width; ++i)
    {
        float position = static_cast<float>(i) / static_cast<float>(width - 1);

        if (position >= points[next_index].position)
        {
            prev_index = next_index;
            next_index = std::min(next_index + 1, points.size() - 1);
        }

        const auto& prev_point = points[prev_index];
        const auto& next_point = points[next_index];

        vec3f color = {};
        if (prev_index == next_index)
        {
            color = prev_point.color;
        }
        else
        {
            color = vector::lerp(
                prev_point.color,
                next_point.color,
                (position - prev_point.position) / (next_point.position - prev_point.position));
        }

        data.pixels.push_back(static_cast<char>(color.x * 255.0f));
        data.pixels.push_back(static_cast<char>(color.y * 255.0f));
        data.pixels.push_back(static_cast<char>(color.z * 255.0f));
        data.pixels.push_back(static_cast<char>(255));
    }

    set_texture(texture_loader::load(data));
}
} // namespace violet