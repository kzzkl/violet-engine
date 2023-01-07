#pragma once

#include "physics_interface.hpp"
#include "scene/transform.hpp"
#include <memory>

namespace violet::physics
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
    static constexpr std::uint32_t COLLISION_MASK_ALL = 0xFFFFFFFF;

public:
    rigidbody();

    void type(rigidbody_type type) noexcept { m_type = type; }
    void mass(float mass);
    void linear_damping(float linear_damping);
    void angular_damping(float angular_damping);
    void restitution(float restitution);
    void friction(float friction);
    void shape(collision_shape_interface* shape);
    void collision_group(std::uint32_t group) noexcept { m_collision_group = group; }
    void collision_mask(std::uint32_t mask) noexcept { m_collision_mask = mask; }
    void offset(const math::float4x4& offset) noexcept;

    rigidbody_type type() const noexcept { return m_type; }
    float mass() const noexcept { return m_mass; }
    float linear_damping() const noexcept { return m_linear_damping; }
    float angular_damping() const noexcept { return m_angular_damping; }
    float restitution() const noexcept { return m_restitution; }
    float friction() const noexcept { return m_friction; }
    collision_shape_interface* shape() const noexcept { return m_shape; }
    std::uint32_t collision_group() const noexcept { return m_collision_group; }
    std::uint32_t collision_mask() const noexcept { return m_collision_mask; }
    const math::float4x4& offset() const noexcept { return m_offset; }
    const math::float4x4& offset_inverse() const noexcept { return m_offset_inverse; }

    void sync_world(const math::float4x4& rigidbody_transform, scene::transform& transform);

    template <typename T, typename... Args>
    void reflection(Args&&... args)
        requires std::is_base_of_v<rigidbody_transform_reflection, T>
    {
        m_reflection = std::make_unique<T>(std::forward<Args>(args)...);
    }

    rigidbody_interface* interface() const noexcept { return m_interface.get(); }
    void reset_interface(rigidbody_interface* interface) noexcept { m_interface.reset(interface); }

private:
    rigidbody_type m_type;
    float m_mass;
    float m_linear_damping;
    float m_angular_damping;
    float m_restitution;
    float m_friction;

    collision_shape_interface* m_shape;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;

    math::float4x4 m_offset;
    math::float4x4 m_offset_inverse;

    std::unique_ptr<rigidbody_transform_reflection> m_reflection;
    std::unique_ptr<rigidbody_interface> m_interface;
};
} // namespace violet::physics