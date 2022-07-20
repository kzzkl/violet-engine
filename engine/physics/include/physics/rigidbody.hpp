#pragma once

#include "physics_interface.hpp"
#include "scene/transform.hpp"
#include <memory>

namespace ash::physics
{
class rigidbody;
class rigidbody_transform_reflection
{
public:
    rigidbody_transform_reflection() noexcept = default;
    virtual ~rigidbody_transform_reflection() = default;

    virtual void reflect(
        const math::float4x4& rigidbody_transform,
        const rigidbody& rigidbody,
        scene::transform& transform) const noexcept;
};

class rigidbody
{
public:
    static constexpr std::uint32_t COLLISION_MASK_ALL = -1;

public:
    rigidbody();

    void sync_world(scene::transform& transform);

    rigidbody_type type{rigidbody_type::DYNAMIC};
    float mass{0.0f};
    float linear_dimmer;
    float angular_dimmer;
    float restitution;
    float friction;

    collision_shape_interface* shape;

    std::uint32_t collision_group{1};
    std::uint32_t collision_mask{COLLISION_MASK_ALL};

    math::float4x4 offset{math::matrix::identity()};
    math::float4x4 offset_inverse{math::matrix::identity()};

    std::unique_ptr<rigidbody_interface> interface;

    std::unique_ptr<rigidbody_transform_reflection> reflection;
};
} // namespace ash::physics