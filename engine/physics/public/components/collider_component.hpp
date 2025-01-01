#pragma once

#include "physics/physics_interface.hpp"
#include <vector>

namespace violet
{
struct collider_shape
{
    phy_collision_shape_desc shape;
    mat4f offset;
};

struct collider_component
{
    std::vector<collider_shape> shapes;
};
} // namespace violet