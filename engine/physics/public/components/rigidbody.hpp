#pragma once

#include "core/node/node.hpp"
#include "physics/physics_interface.hpp"

namespace violet
{
class joint;
class rigidbody
{
public:
    rigidbody();
    rigidbody(const rigidbody&) = delete;
    rigidbody(rigidbody&& other);
    ~rigidbody();

    void set_type(pei_rigidbody_type type);
    void set_shape(pei_collision_shape* shape);
    void set_mass(float mass) noexcept;
    void set_damping(float linear_damping, float angular_damping);
    void set_restitution(float restitution);
    void set_friction(float friction);

    void set_collision_group(std::size_t group) { m_collision_group = group; }
    std::size_t get_collision_group() const noexcept { return m_collision_group; }

    void set_collision_mask(std::size_t mask) { m_collision_mask = mask; }
    std::size_t get_collision_mask() const noexcept { return m_collision_mask; }

    void set_transform(const float4x4& transform);
    const float4x4& get_transform() const;

    joint* add_joint();

    void set_updated_flag(bool flag);
    bool get_updated_flag() const;

    rigidbody& operator=(const rigidbody&) = delete;
    rigidbody& operator=(rigidbody&& other);

private:
    friend class physics_world;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;

    pei_rigidbody_desc m_desc;
    pei_rigidbody* m_rigidbody;

    std::vector<std::unique_ptr<joint>> m_joints;

    pei_plugin* m_pei;
};

class joint
{
public:
    void set_target(
        component_ptr<rigidbody> target,
        const float3& position,
        const float4& rotation);

    void set_linear(const float3& min, const float3& max);
    void set_angular(const float3& min, const float3& max);

    void set_spring_enable(std::size_t index, bool enable);
    void set_stiffness(std::size_t index, float stiffness);

private:
    friend class physics_world;

    component_ptr<rigidbody> m_target;

    pei_joint_desc m_desc;
    pei_joint* m_joint;
};
} // namespace violet