#pragma once

#include "math/types.hpp"

namespace violet
{
class phy_motion_state
{
public:
    virtual ~phy_motion_state() = default;

    virtual const mat4f& get_transform() const = 0;
    virtual void set_transform(const mat4f& transform) = 0;
};

enum phy_collision_shape_type
{
    PHY_COLLISION_SHAPE_TYPE_BOX,
    PHY_COLLISION_SHAPE_TYPE_SPHERE,
    PHY_COLLISION_SHAPE_TYPE_CAPSULE,
};

struct phy_collision_shape_desc
{
    phy_collision_shape_type type;
    union
    {
        struct
        {
            float width;
            float height;
            float length;
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

class phy_collision_shape
{
public:
    virtual ~phy_collision_shape() = default;
};

enum phy_rigidbody_type
{
    PHY_RIGIDBODY_TYPE_STATIC,
    PHY_RIGIDBODY_TYPE_DYNAMIC,
    PHY_RIGIDBODY_TYPE_KINEMATIC
};

enum phy_activation_state
{
    PHY_ACTIVATION_STATE_ACTIVE,
    PHY_ACTIVATION_STATE_DISABLE_DEACTIVATION,
    PHY_ACTIVATION_STATE_DISABLE_SIMULATION,
};

struct phy_rigidbody_desc
{
    phy_rigidbody_type type;

    phy_collision_shape* shape;
    float mass;

    float linear_damping;
    float angular_damping;
    float restitution;
    float friction;

    phy_activation_state activation_state;

    std::uint32_t collision_group;
    std::uint32_t collision_mask;

    phy_motion_state* motion_state;
};

class phy_rigidbody
{
public:
    virtual ~phy_rigidbody() = default;

    virtual void set_mass(float mass) = 0;
    virtual void set_damping(float linear_damping, float angular_damping) = 0;
    virtual void set_restitution(float restitution) = 0;
    virtual void set_friction(float friction) = 0;
    virtual void set_shape(phy_collision_shape* shape) = 0;

    virtual void set_angular_velocity(const vec3f& velocity) = 0;
    virtual void set_linear_velocity(const vec3f& velocity) = 0;

    virtual void clear_forces() = 0;

    virtual void set_activation_state(phy_activation_state activation_state) = 0;
    virtual void set_motion_state(phy_motion_state* motion_state) = 0;
};

struct phy_joint_desc
{
    phy_rigidbody* source;
    vec3f source_position;
    vec4f source_rotation;

    phy_rigidbody* target;
    vec3f target_position;
    vec4f target_rotation;
};

class phy_joint
{
public:
    virtual ~phy_joint() = default;

    virtual void set_linear(const vec3f& min, const vec3f& max) = 0;
    virtual void set_angular(const vec3f& min, const vec3f& max) = 0;

    virtual void set_spring_enable(std::size_t index, bool enable) = 0;
    virtual void set_stiffness(std::size_t index, float stiffness) = 0;
    virtual void set_damping(std::size_t index, float damping) = 0;
};

class phy_debug_draw
{
public:
    virtual ~phy_debug_draw() = default;

    virtual void draw_line(const vec3f& start, const vec3f& end, const vec3f& color) = 0;
};

struct phy_world_desc
{
    vec3f gravity;
    phy_debug_draw* debug_draw;
};

class phy_world
{
public:
    virtual ~phy_world() = default;

    virtual void add(phy_rigidbody* rigidbody) = 0;
    virtual void add(phy_joint* joint) = 0;

    virtual void remove(phy_rigidbody* rigidbody) = 0;
    virtual void remove(phy_joint* joint) = 0;

    virtual void simulation(float time_step) = 0;

    virtual void debug() = 0;
};

class phy_plugin
{
public:
    virtual ~phy_plugin() = default;

    virtual phy_world* create_world(const phy_world_desc& desc) = 0;
    virtual void destroy_world(phy_world* world) = 0;

    virtual phy_collision_shape* create_collision_shape(const phy_collision_shape_desc& desc) = 0;
    virtual phy_collision_shape* create_collision_shape(
        const phy_collision_shape* const* child,
        const mat4f* offset,
        std::size_t size) = 0;
    virtual void destroy_collision_shape(phy_collision_shape* collision_shape) = 0;

    virtual phy_rigidbody* create_rigidbody(const phy_rigidbody_desc& desc) = 0;
    virtual void destroy_rigidbody(phy_rigidbody* rigidbody) = 0;

    virtual phy_joint* create_joint(const phy_joint_desc& desc) = 0;
    virtual void destroy_joint(phy_joint* joint) = 0;
};

using phy_create_plugin = phy_plugin* (*)();
using phy_destroy_plugin = void (*)(phy_plugin*);
} // namespace violet