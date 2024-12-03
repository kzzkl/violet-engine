#include "graphics/geometries/sphere_geometry.hpp"
#include "math/vector.hpp"

namespace violet
{
sphere_geometry::sphere_geometry(float radius, std::size_t slice, std::size_t stack)
{
    std::vector<vec3f> position;
    std::vector<vec3f> normal;
    std::vector<std::uint32_t> indexes;

    // Top vertex.
    position.push_back(vec3f{0.0f, radius, 0.0f});
    normal.push_back(vec3f{0.0f, 1.0f, 0.0f});

    for (std::size_t i = 1; i < stack; ++i)
    {
        float phi = math::PI * static_cast<float>(i) / static_cast<float>(stack);
        auto [phi_sin, phi_cos] = math::sin_cos(phi);
        for (std::size_t j = 0; j < slice; ++j)
        {
            float theta = 2.0f * math::PI * static_cast<float>(j) / static_cast<float>(slice);
            auto [theta_sin, theta_cos] = math::sin_cos(theta);

            vec3f p = {
                phi_sin * theta_cos * radius,
                phi_cos * radius,
                phi_sin * theta_sin * radius};
            position.push_back(p);
            normal.push_back(vector::normalize(p));
        }
    }

    // Bottom vertex.
    position.push_back(vec3f{0.0f, -radius, 0.0f});
    normal.push_back(vec3f{0.0f, -1.0f, 0.0f});

    for (std::size_t i = 0; i < slice; ++i)
    {
        // Top triangles.
        indexes.push_back(0);
        indexes.push_back((i + 1) % slice + 1);
        indexes.push_back(i + 1);

        // Bottom triangles.
        indexes.push_back(position.size() - 1);
        indexes.push_back(i + slice * (stack - 2) + 1);
        indexes.push_back((i + 1) % slice + slice * (stack - 2) + 1);
    }

    for (std::size_t i = 0; i < stack - 2; ++i)
    {
        std::size_t i0 = i * slice + 1;
        std::size_t i1 = (i + 1) * slice + 1;

        for (std::size_t j = 0; j < slice; ++j)
        {
            std::uint32_t v0 = i0 + j;
            std::uint32_t v1 = i0 + (j + 1) % slice;
            std::uint32_t v2 = i1 + (j + 1) % slice;
            std::uint32_t v3 = i1 + j;

            indexes.insert(indexes.end(), {v0, v1, v3, v1, v2, v3});
        }
    }

    add_attribute("position", position);
    add_attribute("normal", normal);
    set_indexes(indexes);
}
} // namespace violet