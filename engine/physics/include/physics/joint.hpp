#pragma once

#include "ecs/entity.hpp"
#include "physics_interface.hpp"
#include <array>
#include <memory>

namespace ash::physics
{
class joint
{
public:
    joint();

    void relative_rigidbody(
        ecs::entity rigidbody_a,
        const math::float3& relative_position_a,
        const math::float4& relative_rotation_a,
        ecs::entity rigidbody_b,
        const math::float3& relative_position_b,
        const math::float4& relative_rotation_b);

    void min_linear(const math::float3& min_linear);
    void max_linear(const math::float3& max_linear);

    void min_angular(const math::float3& min_angular);
    void max_angular(const math::float3& max_angular);

    void spring_enable(std::size_t i, bool enable);
    void stiffness(std::size_t i, float stiffness);

    ecs::entity relative_a() const noexcept { return m_relative_a; }
    const math::float3& relative_position_a() const noexcept { return m_relative_position_a; }
    const math::float4& relative_rotation_a() const noexcept { return m_relative_rotation_a; }

    ecs::entity relative_b() const noexcept { return m_relative_b; }
    const math::float3& relative_position_b() const noexcept { return m_relative_position_b; }
    const math::float4& relative_rotation_b() const noexcept { return m_relative_rotation_b; }

    const math::float3& min_linear() const noexcept { return m_min_linear; }
    const math::float3& max_linear() const noexcept { return m_max_linear; }

    const math::float3& min_angular() const noexcept { return m_min_angular; }
    const math::float3& max_angular() const noexcept { return m_max_angular; }

    const std::array<bool, 6>& spring_enable() const noexcept { return m_spring_enable; }
    const std::array<float, 6>& stiffness() const noexcept { return m_stiffness; }

    joint_interface* interface() const noexcept { return m_interface.get(); }
    void reset_interface(joint_interface* interface) noexcept { m_interface.reset(interface); }

private:
    ecs::entity m_relative_a;
    math::float3 m_relative_position_a;
    math::float4 m_relative_rotation_a;
    ecs::entity m_relative_b;
    math::float3 m_relative_position_b;
    math::float4 m_relative_rotation_b;

    math::float3 m_min_linear;
    math::float3 m_max_linear;

    math::float3 m_min_angular;
    math::float3 m_max_angular;

    std::array<bool, 6> m_spring_enable;
    std::array<float, 6> m_stiffness;

    std::unique_ptr<joint_interface> m_interface;
};
} // namespace ash::physics