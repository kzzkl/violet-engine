#include "graphics/geometries/box_geometry.hpp"
#include "math/math.hpp"

namespace violet
{
box_geometry::box_geometry(float width, float height, float depth)
{
    float half_width = width * 0.5f;
    float half_height = height * 0.5f;
    float half_depth = depth * 0.5f;

    std::vector<float3> position = {
        {-half_width, half_height,  half_depth }, // up
        {half_width,  half_height,  half_depth },
        {half_width,  half_height,  -half_depth},
        {-half_width, half_height,  -half_depth},
        {-half_width, -half_height, half_depth }, // bottom
        {half_width,  -half_height, half_depth },
        {half_width,  -half_height, -half_depth},
        {-half_width, -half_height, -half_depth},
        {-half_width, half_height,  half_depth }, // left
        {-half_width, half_height,  -half_depth},
        {-half_width, -half_height, -half_depth},
        {-half_width, -half_height, half_depth },
        {half_width,  half_height,  half_depth }, // right
        {half_width,  half_height,  -half_depth},
        {half_width,  -half_height, -half_depth},
        {half_width,  -half_height, half_depth },
        {-half_width, half_height,  -half_depth}, // forward
        {half_width,  half_height,  -half_depth},
        {half_width,  -half_height, -half_depth},
        {-half_width, -half_height, -half_depth},
        {-half_width, half_height,  half_depth }, // back
        {half_width,  half_height,  half_depth },
        {half_width,  -half_height, half_depth },
        {-half_width, -half_height, half_depth }
    };

    std::vector<float3> normal = {
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

    std::vector<std::uint32_t> indices = {
        0,  2,  1,  0,  3,  2,  // up
        4,  5,  6,  4,  6,  7,  // down
        8,  10, 9,  8,  11, 10, // left
        12, 13, 14, 12, 14, 15, // right
        16, 18, 17, 16, 19, 18, // forward
        20, 21, 22, 20, 22, 23  // back
    };

    add_attribute("position", std::span(position.data(), position.size()));
    add_attribute("normal", normal);
    set_indices(indices);
}
} // namespace violet