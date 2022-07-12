#pragma once

#include "math/math.hpp"
#include <vector>

namespace ash::graphics
{
struct geometry_data
{
    std::vector<math::float3> position;
    std::vector<math::float3> normal;
    std::vector<std::uint32_t> indices;
};

class geometry
{
public:
    static geometry_data box(float x, float y, float z) noexcept;
    static geometry_data shpere(float diameter, std::size_t slice, std::size_t stack) noexcept;
};
} // namespace ash::graphics