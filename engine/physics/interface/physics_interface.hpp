#pragma once

#include "math/math.hpp"
#include "plugin_interface.hpp"

namespace violet::physics
{
enum class collision_shape_type
{
    BOX,
    SPHERE,
    CAPSULE
};

struct collision_shape_desc
{
    collision_shape_type type;
    union {
        struct
        {
            float length;
            float height;
            float width;
        } box;

        struct
        {
            float radius;
        } sphere;

        struct
        {
            float radius;
            float height;
        } capsule;
    };
};

class collision_shape_interface
{
public:
    virtual ~collision_shape_interface() = default;
};

enum class rigidbody_type
{
    STATIC,
    DYNAMIC,
    KINEMATIC
};

struct rigidbody_desc
{
    rigidbody_type type;

    collision_shape_interface* shape;
    float mass;

    float linear_damping;
    float angular_damping;
    float restitution;
    float friction;

    math::float4x4 initial_transform;
};

class rigidbody_interface
{
public:
    virtual ~rigidbody_interface() = default;

    virtual void mass(float mass) = 0;
    virtual void damping(float linear_damping, float angular_damping) = 0;
    virtual void restitution(float restitution) = 0;
    virtual void friction(float friction) = 0;
    virtual void shape(collision_shape_interface* shape) = 0;

    virtual const math::float4x4& transform() const = 0;
    virtual void transform(const math::float4x4& world) = 0;

    virtual void angular_velocity(const math::float3& velocity) = 0;
    virtual void linear_velocity(const math::float3& velocity) = 0;

    virtual void clear_forces() = 0;

    std::size_t user_data_index;
};

struct joint_desc
{
    rigidbody_interface* rigidbody_a;
    rigidbody_interface* rigidbody_b;

    math::float3 relative_position_a;
    math::float4 relative_rotation_a;
    math::float3 relative_position_b;
    math::float4 relative_rotation_b;

    math::float3 min_linear;
    math::float3 max_linear;

    math::float3 min_angular;
    math::float3 max_angular;

    bool spring_enable[6];
    float stiffness[6];
};

class joint_interface
{
public:
    virtual ~joint_interface() = default;

    virtual void min_linear(const math::float3& linear) = 0;
    virtual void max_linear(const math::float3& linear) = 0;

    virtual void min_angular(const math::float3& angular) = 0;
    virtual void max_angular(const math::float3& angular) = 0;

    virtual void spring_enable(std::size_t i, bool enable) = 0;
    virtual void stiffness(std::size_t i, float stiffness) = 0;
};

struct world_desc
{
    math::float3 gravity;
};

class world_interface
{
public:
    virtual ~world_interface() = default;

    virtual void add(
        rigidbody_interface* rigidbody,
        std::uint32_t collision_group,
        std::uint32_t collision_mask) = 0;
    virtual void add(joint_interface* joint) = 0;

    virtual void remove(rigidbody_interface* rigidbody) = 0;

    virtual void simulation(float time_step) = 0;
    virtual void debug() {}

    virtual rigidbody_interface* updated_rigidbody() = 0;
};

class debug_draw_interface
{
public:
    virtual ~debug_draw_interface() = default;

    virtual void draw_line(
        const math::float3& start,
        const math::float3& end,
        const math::float3& color) = 0;
};
using debug_draw = debug_draw_interface;

class factory_interface
{
public:
    virtual ~factory_interface() = default;

    virtual world_interface* make_world(
        const world_desc& desc,
        debug_draw_interface* debug_draw = nullptr) = 0;
    virtual collision_shape_interface* make_collision_shape(const collision_shape_desc& desc) = 0;
    virtual collision_shape_interface* make_collision_shape(
        const collision_shape_interface* const* child,
        const math::float4x4* offset,
        std::size_t size) = 0;
    virtual rigidbody_interface* make_rigidbody(const rigidbody_desc& desc) = 0;
    virtual joint_interface* make_joint(const joint_desc& desc) = 0;
};
using factory = factory_interface;

using make_factory = factory_interface* (*)();
} // namespace violet::physics