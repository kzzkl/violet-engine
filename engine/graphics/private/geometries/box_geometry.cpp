#include "graphics/geometries/box_geometry.hpp"
#include "math/math.hpp"

namespace violet
{
box_geometry::box_geometry(render_device* device, float width, float height, float depth)
    : geometry(device)
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
        0,  1,  2,  0,  2,  3,  // up
        4,  6,  5,  4,  7,  6,  // down
        8,  9,  10, 8,  10, 11, // left
        12, 14, 13, 12, 15, 14, // right
        16, 17, 18, 16, 18, 19, // forward
        20, 22, 21, 20, 23, 22  // back
    };

    add_attribute("position", position);
    add_attribute("normal", normal);
    set_indices(indices);
}
} // namespace violet