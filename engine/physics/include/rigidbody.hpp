#pragma once

#include "ecs.hpp"
#include "physics_interface.hpp"
#include "transform.hpp"

namespace ash::physics
{
struct rigidbody
{
    ecs::entity relation{ecs::INVALID_ENTITY};

    rigidbody_type type{rigidbody_type::DYNAMIC};
    float mass{0.0f};
    float linear_dimmer;
    float angular_dimmer;
    float restitution;
    float friction;

    collision_shape_interface* shape;

    std::uint32_t collision_group;
    std::uint32_t collision_mask;

    math::float4x4 offset;
    math::float4x4 offset_inverse;

    bool in_world{false};

    std::unique_ptr<rigidbody_interface> interface;
};
} // namespace ash::physics