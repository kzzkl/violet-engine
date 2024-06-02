#pragma once

#include "ecs/actor.hpp"
#include "physics/physics_context.hpp"

namespace violet
{
class rigidbody_reflector
{
public:
    ~rigidbody_reflector() = default;

    virtual float4x4 reflect(const float4x4& rigidbody_world, const float4x4& transform_world);
};

class joint;
class rigidbody
{
public:
    rigidbody(physics_context* context) noexcept;
    rigidbody(const rigidbody&) = delete;
    rigidbody(rigidbody&& other) noexcept;
    ~rigidbody();

    void set_type(phy_rigidbody_type type);
    phy_rigidbody_type get_type() const noexcept { return m_desc.type; }

    void set_shape(phy_collision_shape* shape);
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

    void set_offset(const float4x4& offset) noexcept;
    const float4x4& get_offset() const noexcept { return m_offset; }
    const float4x4& get_offset_inverse() const noexcept { return m_offset_inverse; }

    void set_activation_state(phy_rigidbody_activation_state activation_state);

    void clear_forces();

    joint* add_joint(
        component_ptr<rigidbody> target,
        const float3& source_position = {},
        const float4& source_rotation = {0.0f, 0.0f, 0.0f, 1.0f},
        const float3& target_position = {},
        const float4& target_rotation = {0.0f, 0.0f, 0.0f, 1.0f});
    void remove_joint(joint* joint);

    void set_updated_flag(bool flag);
    bool get_updated_flag() const;

    template <typename T, typename... Args>
    void set_reflector(Args&&... args)
    {
        m_reflector = std::make_unique<T>(std::forward<Args>(args)...);
    }
    rigidbody_reflector* get_reflector() const noexcept { return m_reflector.get(); }

    phy_rigidbody* get_rigidbody();
    std::vector<phy_joint*> get_joints();

    void set_world(phy_world* world);

    rigidbody& operator=(const rigidbody&) = delete;
    rigidbody& operator=(rigidbody&& other) noexcept;

private:
    friend class joint;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;

    float4x4 m_offset;
    float4x4 m_offset_inverse;

    phy_rigidbody_desc m_desc;
    phy_ptr<phy_rigidbody> m_rigidbody;

    std::vector<std::unique_ptr<joint>> m_joints;
    std::vector<joint*> m_slave_joints;

    std::unique_ptr<rigidbody_reflector> m_reflector;

    phy_world* m_world;
    physics_context* m_context;
};

class joint
{
public:
    joint(
        rigidbody* source,
        component_ptr<rigidbody> target,
        const float3& source_position,
        const float4& source_rotation,
        const float3& target_position,
        const float4& target_rotation,
        physics_context* context);
    joint(const joint&) = delete;
    ~joint();

    void set_linear(const float3& min, const float3& max);
    void set_angular(const float3& min, const float3& max);

    void set_spring_enable(std::size_t index, bool enable);
    void set_stiffness(std::size_t index, float stiffness);
    void set_damping(std::size_t index, float damping);

    phy_joint* get_joint() const noexcept { return m_joint.get(); }

    joint& operator=(const joint&) = delete;

private:
    friend class rigidbody;

    rigidbody* m_source;
    component_ptr<rigidbody> m_target;
    phy_ptr<phy_joint> m_joint;
};
} // namespace violet