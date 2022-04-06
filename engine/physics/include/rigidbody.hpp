#pragma once

#include "ecs.hpp"
#include "physics_exports.hpp"
#include "physics_interface.hpp"
#include "transform.hpp"

namespace ash::physics
{
class PHYSICS_API rigidbody
{
public:
    rigidbody();

    void mass(float mass) noexcept;
    float mass() const noexcept { return m_mass; }

    void linear_dimmer(float linear_dimmer) noexcept { m_linear_dimmer = linear_dimmer; }
    float linear_dimmer() noexcept { return m_linear_dimmer; }

    void angular_dimmer(float angular_dimmer) noexcept { m_angular_dimmer = angular_dimmer; }
    float angular_dimmer() noexcept { return m_angular_dimmer; }

    void restitution(float restitution) noexcept { m_restitution = restitution; }
    float restitution() noexcept { return m_restitution; }

    void friction(float friction) noexcept { m_friction = friction; }
    float friction() noexcept { return m_friction; }

    void shape(collision_shape_interface* shape) noexcept { m_shape = shape; }
    collision_shape_interface* shape() const noexcept { return m_shape; }

    void offset(const math::float4x4& offset) noexcept;
    void offset(const math::float4x4_simd& offset) noexcept;
    const math::float4x4& offset() const noexcept { return m_offset; }
    const math::float4x4& offset_inverse() const noexcept { return m_offset_inverse; }

    void in_world(bool in_world) noexcept { m_in_world = in_world; }
    bool in_world() const noexcept { return m_in_world; }

    void collision_group(std::uint32_t collision_group) noexcept
    {
        m_collision_group = collision_group;
    }
    std::uint32_t collision_group() const noexcept { return m_collision_group; }

    void collision_mask(std::uint32_t collision_mask) noexcept
    {
        m_collision_mask = collision_mask;
    }
    std::uint32_t collision_mask() const noexcept { return m_collision_mask; }

    std::unique_ptr<rigidbody_interface> interface;
    ash::ecs::component_handle<ash::scene::transform> node;

private:
    float m_mass;
    float m_linear_dimmer;
    float m_angular_dimmer;
    float m_restitution;
    float m_friction;

    collision_shape_interface* m_shape;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;

    math::float4x4 m_offset;
    math::float4x4 m_offset_inverse;

    bool m_in_world;
};
} // namespace ash::physics

namespace ash::ecs
{
template <>
struct component_trait<ash::physics::rigidbody>
{
    static constexpr std::size_t id = ash::uuid("308b46bf-898e-40fe-bd67-1d0d8e5a1aa3").hash();
};
} // namespace ash::ecs