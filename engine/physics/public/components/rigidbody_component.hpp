#pragma once

#include "physics/physics_interface.hpp"
#include <functional>

namespace violet
{
struct rigidbody_component
{
    phy_rigidbody_type type;

    float mass;
    float linear_damping;
    float angular_damping;
    float restitution;
    float friction;

    phy_activation_state activation_state;

    std::uint32_t collision_group{0xFFFFFFFF};
    std::uint32_t collision_mask{0xFFFFFFFF};

    mat4f offset;

    std::function<mat4f(const mat4f&, const mat4f&)> transform_reflector;
};
} // namespace violet