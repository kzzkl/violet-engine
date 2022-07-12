#include "graphics/geometry.hpp"

namespace ash::graphics
{
geometry_data geometry::box(float x, float y, float z) noexcept
{
    float half_x = x * 0.5f;
    float half_y = y * 0.5f;
    float half_z = z * 0.5f;

    geometry_data result;
    result.position = {
        {-half_x, half_y,  half_z }, // up
        {half_x,  half_y,  half_z },
        {half_x,  half_y,  -half_z},
        {-half_x, half_y,  -half_z},

        {-half_x, -half_y, half_z }, // bottom
        {half_x,  -half_y, half_z },
        {half_x,  -half_y, -half_z},
        {-half_x, -half_y, -half_z},

        {-half_x, half_y,  half_z }, // left
        {-half_x, half_y,  -half_z},
        {-half_x, -half_y, -half_z},
        {-half_x, -half_y, half_z },

        {half_x,  half_y,  half_z }, // right
        {half_x,  half_y,  -half_z},
        {half_x,  -half_y, -half_z},
        {half_x,  -half_y, half_z },

        {-half_x, half_y,  -half_z}, // forward
        {half_x,  half_y,  -half_z},
        {half_x,  -half_y, -half_z},
        {-half_x, -half_y, -half_z},

        {-half_x, half_y,  half_z }, // back
        {half_x,  half_y,  half_z },
        {half_x,  -half_y, half_z },
        {-half_x, -half_y, half_z }
    };
    result.normal = {
        {0.0f,  1.0f,  0.0f }, // up
        {0.0f,  1.0f,  0.0f },
        {0.0f,  1.0f,  0.0f },
        {0.0f,  1.0f,  0.0f },

        {0.0f,  -1.0f, 0.0f }, // bottom
        {0.0f,  -1.0f, 0.0f },
        {0.0f,  -1.0f, 0.0f },
        {0.0f,  -1.0f, 0.0f },

        {-1.0f, 0.0f,  0.0f }, // left
        {-1.0f, 0.0f,  0.0f },
        {-1.0f, 0.0f,  0.0f },
        {-1.0f, 0.0f,  0.0f },

        {1.0f,  0.0f,  0.0f }, // right
        {1.0f,  0.0f,  0.0f },
        {1.0f,  0.0f,  0.0f },
        {1.0f,  0.0f,  0.0f },

        {0.0f,  0.0f,  -1.0f}, // forward
        {0.0f,  0.0f,  -1.0f},
        {0.0f,  0.0f,  -1.0f},
        {0.0f,  0.0f,  -1.0f},

        {0.0f,  0.0f,  1.0f }, // back
        {0.0f,  0.0f,  1.0f },
        {0.0f,  0.0f,  1.0f },
        {0.0f,  0.0f,  1.0f }
    };

    result.indices = {
        0,  1,  2,  0,  2,  3,  // up
        4,  6,  5,  4,  7,  6,  // down
        8,  9,  10, 8,  10, 11, // left
        12, 14, 13, 12, 15, 14, // right
        16, 17, 18, 16, 18, 19, // forward
        20, 22, 21, 20, 23, 22  // back
    };

    return result;
}

geometry_data geometry::shpere(float diameter, std::size_t slice, std::size_t stack) noexcept
{
    geometry_data result;

    float radius = diameter * 0.5f;

    // Top vertex.
    result.position.emplace_back(math::float3{0.0f, radius, 0.0f});
    result.normal.emplace_back(math::float3{0.0f, 1.0f, 0.0f});

    for (std::size_t i = 1; i < stack; ++i)
    {
        float phi = math::PI * static_cast<float>(i) / static_cast<float>(stack);
        auto [phi_sin, phi_cos] = math::sin_cos(phi);
        for (std::size_t j = 0; j < slice; ++j)
        {
            float theta = 2.0f * math::PI * static_cast<float>(j) / static_cast<float>(slice);
            auto [theta_sin, theta_cos] = math::sin_cos(theta);

            math::float3 position = {
                phi_sin * theta_cos * radius,
                phi_cos * radius,
                phi_sin * theta_sin * radius};
            result.position.push_back(position);
            result.normal.push_back(math::vector::normalize(position));
        }
    }

    // Bottom vertex.
    result.position.emplace_back(math::float3{0.0f, -radius, 0.0f});
    result.normal.emplace_back(math::float3{0.0f, -1.0f, 0.0f});

    for (std::size_t i = 0; i < slice; ++i)
    {
        // Top triangles.
        result.indices.push_back(0);
        result.indices.push_back((i + 1) % slice + 1);
        result.indices.push_back(i + 1);

        // Bottom triangles.
        result.indices.push_back(result.position.size() - 1);
        result.indices.push_back(i + slice * (stack - 2) + 1);
        result.indices.push_back((i + 1) % slice + slice * (stack - 2) + 1);
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

            result.indices.insert(result.indices.end(), {v0, v1, v3, v1, v2, v3});
        }
    }

    return result;
}
} // namespace ash::graphics