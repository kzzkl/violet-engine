#pragma once

#include "component.hpp"
#include "math.hpp"

namespace ash::scene
{
struct transform;
struct transform_node
{
    transform* transform;
    bool dirty;
    std::size_t sync_count;

    transform_node* parent;
    std::vector<transform_node*> children;
};

struct transform
{
    transform();
    transform(const transform&) = delete;
    transform(transform&& other) noexcept;

    void mark_write();

    transform& operator=(const transform&) = delete;
    transform& operator=(transform&& other) noexcept;

    math::float3 position;
    math::float4 rotation;
    math::float3 scaling;

    math::float4x4 parent_matrix;
    math::float4x4 world_matrix;

    std::unique_ptr<transform_node> node;
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
