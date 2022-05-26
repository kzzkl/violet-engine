#pragma once

#include "math/math.hpp"
#include <vector>

namespace ash::graphics
{
using geometry_index = std::uint16_t;

struct geometry_data
{
    std::vector<math::float3> position;
    std::vector<math::float3> normal;
    std::vector<geometry_index> indices;
};

class geometry
{
public:
    static geometry_data box(float x, float y, float z);
};
} // namespace ash::graphics