#pragma once

#include "component.hpp"
#include "math.hpp"
#include "scene_node.hpp"

namespace ash::scene
{
struct transform
{
    math::float3 position;
    math::float4 rotation;
    math::float3 scaling;

    std::unique_ptr<scene_node> node;
    scene_node* parent;
};
} // namespace ash::scene

namespace ash::ecs
{
template <>
struct component_trait<scene::transform>
{
    static constexpr std::size_t id = uuid("a439dd80-979b-4bf9-a540-43eeef9e556d").hash();
};
} // namespace ash::ecs
