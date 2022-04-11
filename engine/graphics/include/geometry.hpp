#pragma once

#include "math.hpp"
#include <vector>

namespace ash::graphics
{
struct geometry_vertex
{
    math::float3 position;
    math::float3 normal;
};

using geometry_index = std::uint16_t;

struct geometry_data
{
    std::vector<geometry_vertex> vertices;
    std::vector<geometry_index> indices;
};

class geometry
{
public:
    static geometry_data box(float x, float y, float z);
};
} // namespace ash::graphics