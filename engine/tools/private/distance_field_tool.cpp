#include "tools/distance_field_tool.hpp"
#include "algorithm/bvh.hpp"
#include "graphics/distance_field/sdf_common.hpp"
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

class distance_field_tool_impl
{
public:
    using distance_field_input = distance_field_tool::input;
    using distance_field_output = distance_field_tool::output;

    distance_field_output generate(const distance_field_input& input)
    {
        assert(input.voxel_density > 0.0f);

        initialize_sign_test_directions();

        build_bvh(input.positions, input.indexes);

        box3f mesh_bounds = m_bvh.get_bounds();

        vec3f mesh_bounds_center = box::get_center(mesh_bounds);
        vec3f mesh_bounds_extent = vector::max(box::get_extent(mesh_bounds), vec3f(0.01f));

        mesh_bounds.min = mesh_bounds_center - mesh_bounds_extent / 2.0f;
        mesh_bounds.max = mesh_bounds_center + mesh_bounds_extent / 2.0f;

        std::uint32_t max_brick_count =
            std::max(1u, static_cast<std::uint32_t>(std::floor(128.0f / SDF_UNIQUE_BRICK_SIZE)));

        m_brick_count = vector::clamp(
            vec3u(vector::round(mesh_bounds_extent * input.voxel_density / SDF_UNIQUE_BRICK_SIZE)),
            vec3u(1),
            vec3u(max_brick_count));

        m_voxel_extent = mesh_bounds_extent / (vec3f(m_brick_count) * SDF_UNIQUE_BRICK_SIZE - 2.0f);

        m_brick_extent = m_voxel_extent * SDF_UNIQUE_BRICK_SIZE;

        m_volume_bounds.min = mesh_bounds.min - m_voxel_extent;
        m_volume_bounds.max = mesh_bounds.max + m_voxel_extent;

        m_max_distance = vector::length(m_voxel_extent) * SDF_MAX_DISTANCE_VOXEL_COUNT;

        std::size_t total_brick_count =
            static_cast<std::size_t>(m_brick_count.x) * m_brick_count.y * m_brick_count.z;
        m_brick_table.reserve(total_brick_count);

        m_expected_brick_count = std::max(total_brick_count / 4, 1ull);
        m_brick_data.resize(m_expected_brick_count * SDF_BRICK_VOXEL_COUNT);

        for (std::uint32_t z = 0; z < m_brick_count.z; ++z)
        {
            for (std::uint32_t y = 0; y < m_brick_count.y; ++y)
            {
                for (std::uint32_t x = 0; x < m_brick_count.x; ++x)
                {
                    vec3f brick_position = vec3f(
                        static_cast<float>(x) * m_brick_extent.x,
                        static_cast<float>(y) * m_brick_extent.y,
                        static_cast<float>(z) * m_brick_extent.z);
                    brick_position += m_volume_bounds.min;

                    build_brick(brick_position);
                }
            }
        }

        return {
            .brick_count = m_brick_count,
            .volume_bounds = m_volume_bounds,
            .brick_table = std::move(m_brick_table),
            .brick_data = std::move(m_brick_data),
        };
    }

private:
    void initialize_sign_test_directions()
    {
        std::size_t sample_count = 8;
        m_sign_test_directions.reserve(sample_count * sample_count);

        for (std::uint32_t x = 0; x < sample_count; ++x)
        {
            for (std::uint32_t y = 0; y < sample_count; ++y)
            {
                float sample_x = (static_cast<float>(x) + 0.5f) / static_cast<float>(sample_count);
                float sample_y =
                    ((static_cast<float>(y) + 0.5f) / static_cast<float>(sample_count) * 2.0f) -
                    1.0f;

                const float phi = sample_x * math::TWO_PI;
                const float theta = std::acosf(sample_y);
                m_sign_test_directions.emplace_back(
                    sin(theta) * cos(phi),
                    -cos(theta),
                    sin(theta) * sin(phi));
            }
        }
    }

    void build_bvh(std::span<const vec3f> positions, std::span<const std::uint32_t> indexes)
    {
        m_bvh.reserve(indexes.size() / 3);

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

            m_bvh.add_primitive(primitive);
        }

        m_bvh.build();
    }

    void build_brick(const vec3f& brick_position)
    {
        if (m_expected_brick_count < m_actual_brick_count + 1)
        {
            m_expected_brick_count *= 2;
            m_brick_data.resize(m_expected_brick_count * SDF_BRICK_VOXEL_COUNT);
        }

        std::size_t data_offset = m_actual_brick_count * SDF_BRICK_VOXEL_COUNT;

        float min_distance = m_max_distance;

        for (std::uint32_t z = 0; z < SDF_BRICK_SIZE; ++z)
        {
            for (std::uint32_t y = 0; y < SDF_BRICK_SIZE; ++y)
            {
                for (std::uint32_t x = 0; x < SDF_BRICK_SIZE; ++x)
                {
                    vec3f p = vector::mul(
                        m_voxel_extent,
                        vec3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)));
                    p = vector::add(p, brick_position);

                    auto [primitive_index, distance] = m_bvh.get_distance(p, m_max_distance);
                    if (primitive_index != BVH_INVALID_PRIMITIVE_INDEX)
                    {
                        distance *= get_distance_sign(p);
                        m_brick_data[data_offset] = encode_distance(distance);

                        min_distance = std::min(min_distance, distance);
                    }
                    else
                    {
                        m_brick_data[data_offset] = 255;
                    }

                    ++data_offset;
                }
            }
        }

        if (min_distance < m_max_distance)
        {
            m_brick_table.push_back(static_cast<std::uint32_t>(m_actual_brick_count));
            ++m_actual_brick_count;
        }
        else
        {
            m_brick_table.push_back(SDF_INVALID_BRICK);
        }
    }

    float get_distance_sign(const vec3f& position) const
    {
        std::uint32_t hit_back = 0;
        std::uint32_t hit_front = 0;

        for (const auto& direction : m_sign_test_directions)
        {
            ray3f ray = {
                .origin = position,
                .direction = direction,
            };

            std::size_t nearest_primitive_index = 0xFFFFFFFF;
            float nearest_distance = std::numeric_limits<float>::max();
            for (auto& [offset, hit] : m_bvh.intersect(ray, m_max_distance))
            {
                if (hit.enter < nearest_distance)
                {
                    nearest_primitive_index = offset;
                    nearest_distance = hit.enter;
                }
            }

            if (nearest_primitive_index != 0xFFFFFFFF)
            {
                const auto& primitive = m_bvh.get_primitive(nearest_primitive_index);

                const vec3f& p0 = primitive.positions[primitive.indexes[0]];
                const vec3f& p1 = primitive.positions[primitive.indexes[1]];
                const vec3f& p2 = primitive.positions[primitive.indexes[2]];

                vec3f normal = vector::normalize(vector::cross(p2 - p0, p1 - p0));

                if (vector::dot(normal, direction) > 0)
                {
                    ++hit_back;
                }
                else
                {
                    ++hit_front;
                }
            }
        }

        return hit_front >= hit_back ? 1.0f : -1.0f;
    }

    std::uint8_t encode_distance(float distance) const
    {
        return static_cast<std::uint8_t>(std::clamp(
            std::round(((distance / m_max_distance * 0.5f) + 0.5f) * 255.0f),
            0.0f,
            255.0f));
    }

    bvh<triangle_primitive> m_bvh;

    box3f m_volume_bounds;

    vec3u m_brick_count;
    vec3f m_brick_extent;

    vec3f m_voxel_extent;

    std::vector<std::uint32_t> m_brick_table;
    std::vector<std::uint8_t> m_brick_data;

    std::size_t m_expected_brick_count;
    std::size_t m_actual_brick_count{0};

    std::vector<vec3f> m_sign_test_directions;

    float m_max_distance;
};

distance_field_tool::output distance_field_tool::generate(const input& input)
{
    distance_field_tool_impl impl;
    return impl.generate(input);
}

bool distance_field_tool::test(const input& input)
{
    const auto indexes = input.indexes;
    const auto positions = input.positions;

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

    std::uint32_t width = 512;
    std::uint32_t height = 512;

    vec3f origin = {10.0f, 8.0f, 0.0f};
    mat4f matrix_v = matrix::look_at(origin, vec3f{0.0f, 0.0f, 0.0f}, vec3f{0.0f, -1.0f, 0.0f});
    mat4f matrix_p = matrix::perspective(math::to_radians(45.0f), 1.0f, 0.1f, 100.0f);
    mat4f matrix_vp = matrix::mul(matrix_v, matrix_p);

    std::ofstream fout("test.ppm");
    fout << "P3\n" << width << " " << height << "\n255\n";

    vec3f light_dir = vector::normalize(vec3f{1.0f, 1.0f, 1.0f});

    for (std::uint32_t i = 0; i < height; ++i)
    {
        for (std::uint32_t j = 0; j < width; ++j)
        {
            vec4f ndc = {
                ((static_cast<float>(i) + 0.5f) / static_cast<float>(width) * 2.0f) - 1.0f,
                ((static_cast<float>(j) + 0.5f) / static_cast<float>(height) * 2.0f) - 1.0f,
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