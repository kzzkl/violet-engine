#pragma once

#include "entity.hpp"
#include "physics_interface.hpp"
#include <memory>

namespace ash::physics
{
struct joint
{
    ecs::entity relation_a;
    ecs::entity relation_b;

    math::float3 relative_position_a;
    math::float4 relative_rotation_a;
    math::float3 relative_position_b;
    math::float4 relative_rotation_b;

    math::float3 min_linear;
    math::float3 max_linear;

    math::float3 min_angular;
    math::float3 max_angular;

    math::float3 spring_translate_factor;
    math::float3 spring_rotate_factor;

    std::unique_ptr<joint_interface> interface;
};
} // namespace ash::physics