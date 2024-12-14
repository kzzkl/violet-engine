#pragma once

#include "physics/physics_interface.hpp"
#include <functional>

namespace violet
{
struct rigidbody_component
{
    phy_rigidbody_type type;

    float mass;
    float linear_damping;
    float angular_damping;
    float restitution;
    float friction;

    phy_activation_state activation_state;

    std::uint32_t collision_group{0xFFFFFFFF};
    std::uint32_t collision_mask{0xFFFFFFFF};

    mat4f offset;

    std::function<mat4f(const mat4f&, const mat4f&)> transform_reflector;
};
/*
class rigidbody_reflector
{
public:
    ~rigidbody_reflector() = default;

    virtual mat4f reflect(const mat4f& rigidbody_world, const mat4f& transform_world);
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
    phy_rigidbody_type get_type() const noexcept
    {
        return m_desc.type;
    }

    void set_shape(phy_collision_shape* shape);
    void set_mass(float mass) noexcept;
    void set_damping(float linear_damping, float angular_damping);
    void set_restitution(float restitution);
    void set_friction(float friction);

    void set_collision_group(std::size_t group)
    {
        m_collision_group = group;
    }
    std::size_t get_collision_group() const noexcept
    {
        return m_collision_group;
    }

    void set_collision_mask(std::size_t mask)
    {
        m_collision_mask = mask;
    }
    std::size_t get_collision_mask() const noexcept
    {
        return m_collision_mask;
    }

    void set_transform(const mat4f& transform);
    const mat4f& get_transform() const;

    void set_offset(const mat4f& offset) noexcept;
    const mat4f& get_offset() const noexcept
    {
        return m_offset;
    }
    const mat4f& get_offset_inverse() const noexcept
    {
        return m_offset_inverse;
    }

    void set_activation_state(phy_rigidbody_activation_state activation_state);

    void clear_forces();

    joint* add_joint(
        component_ptr<rigidbody> target,
        const vec3f& source_position = {},
        const vec4f& source_rotation = {0.0f, 0.0f, 0.0f, 1.0f},
        const vec3f& target_position = {},
        const vec4f& target_rotation = {0.0f, 0.0f, 0.0f, 1.0f});
    void remove_joint(joint* joint);

    void set_updated_flag(bool flag);
    bool get_updated_flag() const;

    template <typename T, typename... Args>
    void set_reflector(Args&&... args)
    {
        m_reflector = std::make_unique<T>(std::forward<Args>(args)...);
    }
    rigidbody_reflector* get_reflector() const noexcept
    {
        return m_reflector.get();
    }

    phy_rigidbody* get_rigidbody();
    std::vector<phy_joint*> get_joints();

    void set_world(phy_world* world);

    rigidbody& operator=(const rigidbody&) = delete;
    rigidbody& operator=(rigidbody&& other) noexcept;

private:
    friend class joint;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;

    mat4f m_offset;
    mat4f m_offset_inverse;

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
        const vec3f& source_position,
        const vec4f& source_rotation,
        const vec3f& target_position,
        const vec4f& target_rotation,
        physics_context* context);
    joint(const joint&) = delete;
    ~joint();

    void set_linear(const vec3f& min, const vec3f& max);
    void set_angular(const vec3f& min, const vec3f& max);

    void set_spring_enable(std::size_t index, bool enable);
    void set_stiffness(std::size_t index, float stiffness);
    void set_damping(std::size_t index, float damping);

    phy_joint* get_joint() const noexcept
    {
        return m_joint.get();
    }

    joint& operator=(const joint&) = delete;

private:
    friend class rigidbody;

    rigidbody* m_source;
    component_ptr<rigidbody> m_target;
    phy_ptr<phy_joint> m_joint;
};
*/
} // namespace violet