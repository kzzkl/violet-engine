#pragma once

#include "entity.hpp"
#include "math.hpp"

namespace ash::scene
{
struct transform
{
    transform();

    math::float3 position;
    math::float4 rotation;
    math::float3 scaling;

    math::float4x4 parent_matrix;
    math::float4x4 world_matrix;

    bool dirty;
    std::size_t sync_count;

    ash::ecs::entity parent;
    std::vector<ash::ecs::entity> children;
};
} // namespace ash::scene