#include "tools/distance_field_tool.hpp"
#include "algorithm/bvh.hpp"
#include "math/ray.hpp"
#include "math/triangle.hpp"
#include <cstddef>
#include <fstream>

namespace violet
{
struct triangle_primitive
{
    box3f bounds;
    vec3f centroid;

    const vec3f* positions;
    const std::uint32_t* indexes;

    const box3f& get_bounds() const noexcept
    {
        return bounds;
    }

    const vec3f& get_centroid() const noexcept
    {
        return centroid;
    }

    ray3f::hit_result intersect(const ray3f& ray) const noexcept
    {
        return ray.intersect(positions[indexes[0]], positions[indexes[1]], positions[indexes[2]]);
    }

    float get_distance_sq(const vec3f& p) const noexcept
    {
        return triangle::get_distance_sq(
            positions[indexes[0]],
            positions[indexes[1]],
            positions[indexes[2]],
            p);
    }
};

bool distance_field_tool::generate(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes,
    texture_data& distance_field)
{
    bvh<triangle_primitive> bvh(indexes.size() / 3);
    for (std::size_t i = 0; i < indexes.size(); i += 3)
    {
        const vec3f& p0 = positions[indexes[i]];
        const vec3f& p1 = positions[indexes[i + 1]];
        const vec3f& p2 = positions[indexes[i + 2]];

        triangle_primitive primitive = {};
        box::expand(primitive.bounds, p0);
        box::expand(primitive.bounds, p1);
        box::expand(primitive.bounds, p2);
        primitive.centroid = box::get_center(primitive.bounds);

        primitive.positions = positions.data();
        primitive.indexes = indexes.data() + i;

        bvh.add_primitive(primitive);
    }

    bvh.build();

    rhi_extent extent = {.width = 64, .height = 64, .depth = 64};

    distance_field.format = RHI_FORMAT_R32_FLOAT;
    distance_field.extent = extent;
    distance_field.layer_count = 1;
    distance_field.level_count = 1;
    distance_field.pixels.resize(
        static_cast<std::size_t>(extent.width * extent.height * extent.depth) * sizeof(float));
    auto* pixels = reinterpret_cast<float*>(distance_field.pixels.data());

    box3f bounds = bvh.get_primitive(0).get_bounds();
    vec3f bounds_extent = box::get_extent(bounds);

    for (std::uint32_t z = 0; z < extent.depth; ++z)
    {
        for (std::uint32_t y = 0; y < extent.height; ++y)
        {
            for (std::uint32_t x = 0; x < extent.width; ++x)
            {
                vec3f position = {
                    bounds.min.x + ((static_cast<float>(x) + 0.5f) /
                                    static_cast<float>(extent.width) * bounds_extent.x),
                    bounds.min.y + ((static_cast<float>(y) + 0.5f) /
                                    static_cast<float>(extent.height) * bounds_extent.y),
                    bounds.min.z + ((static_cast<float>(z) + 0.5f) /
                                    static_cast<float>(extent.depth) * bounds_extent.z),
                };

                auto [primitive_index, distance] = bvh.get_distance(position);
                pixels[(z * extent.height * extent.width) + (y * extent.width) + x] = distance;
            }
        }
    }

    return true;
}

bool distance_field_tool::test(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes,
    texture_data& distance_field)
{
    bvh<triangle_primitive> bvh(indexes.size() / 3);
    for (std::size_t i = 0; i < indexes.size(); i += 3)
    {
        const vec3f& p0 = positions[indexes[i]];
        const vec3f& p1 = positions[indexes[i + 1]];
        const vec3f& p2 = positions[indexes[i + 2]];

        triangle_primitive primitive = {};
        box::expand(primitive.bounds, p0);
        box::expand(primitive.bounds, p1);
        box::expand(primitive.bounds, p2);
        primitive.centroid = box::get_center(primitive.bounds);

        primitive.positions = positions.data();
        primitive.indexes = indexes.data() + i;

        bvh.add_primitive(primitive);
    }

    bvh.build();

    rhi_extent extent = {.width = 512, .height = 512};

    vec3f origin = {10.0f, 8.0f, 0.0f};
    mat4f matrix_v = matrix::look_at(origin, vec3f{0.0f, 0.0f, 0.0f}, vec3f{0.0f, -1.0f, 0.0f});
    mat4f matrix_p = matrix::perspective(math::to_radians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4f matrix_vp = matrix::mul(matrix_v, matrix_p);

    std::ofstream fout("test.ppm");
    fout << "P3\n" << extent.width << " " << extent.height << "\n255\n";

    vec3f light_dir = vector::normalize(vec3f{1.0f, 1.0f, 1.0f});

    for (std::uint32_t i = 0; i < extent.height; ++i)
    {
        for (std::uint32_t j = 0; j < extent.width; ++j)
        {
            vec4f ndc = {
                ((static_cast<float>(i) + 0.5f) / static_cast<float>(extent.width) * 2.0f) - 1.0f,
                ((static_cast<float>(j) + 0.5f) / static_cast<float>(extent.height) * 2.0f) - 1.0f,
                1.0f,
                1.0f,
            };

            vec4f clip = matrix::mul(matrix_vp, ndc);
            vec3f direction = vec3f{clip.x, clip.y, clip.z} - origin;
            direction = vector::normalize(direction);

            ray3f ray{.origin = origin, .direction = direction};

            auto hit_primitives = bvh.intersect(ray);
            if (hit_primitives.empty())
            {
                fout << "0 0 0\n";
                continue;
            }

            vec3f normal = {0.0f, 0.0f, 0.0f};
            float t = std::numeric_limits<float>::infinity();
            for (auto [primitive_index, hit_result] : hit_primitives)
            {
                const auto& primitive = bvh.get_primitive(primitive_index);

                const vec3f& p0 = positions[primitive.indexes[0]];
                const vec3f& p1 = positions[primitive.indexes[1]];
                const vec3f& p2 = positions[primitive.indexes[2]];

                if (hit_result.enter < t)
                {
                    t = hit_result.enter;
                    normal = vector::normalize(vector::cross(p2 - p0, p1 - p0));
                }
            }

            if (t == std::numeric_limits<float>::infinity())
            {
                fout << "0 0 0\n";
                continue;
            }

            std::uint32_t intensity =
                static_cast<std::uint32_t>(std::max(0.0f, vector::dot(normal, light_dir)) * 255.0f);

            ray.origin = ray.origin + ray.direction * t;
            ray.direction = light_dir;

            hit_primitives = bvh.intersect(ray);
            if (!hit_primitives.empty())
            {
                intensity = 0;
            }

            fout << intensity << ' ' << intensity << ' ' << intensity << '\n';
        }
    }

    fout.close();

    return true;
}
} // namespace violet