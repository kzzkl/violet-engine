#pragma once

#include "core/plugin_interface.hpp"
#include "math/math.hpp"

namespace violet
{
enum pei_collision_shape_type
{
    PEI_COLLISION_SHAPE_TYPE_BOX,
    PEI_COLLISION_SHAPE_TYPE_SPHERE,
    PEI_COLLISION_SHAPE_TYPE_CAPSULE
};

struct pei_collision_shape_desc
{
    pei_collision_shape_type type;
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

class pei_collision_shape
{
public:
    virtual ~pei_collision_shape() = default;
};

enum pei_rigidbody_type
{
    PEI_RIGIDBODY_TYPE_STATIC,
    PEI_RIGIDBODY_TYPE_DYNAMIC,
    PEI_RIGIDBODY_TYPE_KINEMATIC
};

enum pei_rigidbody_activation_state
{
    PEI_RIGIDBODY_ACTIVATION_STATE_ACTIVE,
    PEI_RIGIDBODY_ACTIVATION_STATE_DISABLE_DEACTIVATION,
    PEI_RIGIDBODY_ACTIVATION_STATE_DISABLE_SIMULATION,
};

struct pei_rigidbody_desc
{
    pei_rigidbody_type type;

    pei_collision_shape* shape;
    float mass;

    float linear_damping;
    float angular_damping;
    float restitution;
    float friction;

    float4x4 initial_transform;

    pei_rigidbody_activation_state activation_state = PEI_RIGIDBODY_ACTIVATION_STATE_ACTIVE;
};

class pei_rigidbody
{
public:
    virtual ~pei_rigidbody() = default;

    virtual void set_mass(float mass) = 0;
    virtual void set_damping(float linear_damping, float angular_damping) = 0;
    virtual void set_restitution(float restitution) = 0;
    virtual void set_friction(float friction) = 0;
    virtual void set_shape(pei_collision_shape* shape) = 0;

    virtual void set_transform(const float4x4& world) = 0;
    virtual const float4x4& get_transform() const = 0;

    virtual void set_angular_velocity(const float3& velocity) = 0;
    virtual void set_linear_velocity(const float3& velocity) = 0;

    virtual void clear_forces() = 0;

    virtual void set_activation_state(pei_rigidbody_activation_state state) = 0;

    virtual void set_updated_flag(bool flag) = 0;
    virtual bool get_updated_flag() const = 0;
};

struct pei_joint_desc
{
    pei_rigidbody* source;
    float3 source_position;
    float4 source_rotation;

    pei_rigidbody* target;
    float3 target_position;
    float4 target_rotation;

    float3 min_linear;
    float3 max_linear;

    float3 min_angular;
    float3 max_angular;

    bool spring_enable[6];
    float stiffness[6];
    float damping[6];
};

class pei_joint
{
public:
    virtual ~pei_joint() = default;

    virtual void set_linear(const float3& min, const float3& max) = 0;
    virtual void set_angular(const float3& min, const float3& max) = 0;

    virtual void set_spring_enable(std::size_t index, bool enable) = 0;
    virtual void set_stiffness(std::size_t index, float stiffness) = 0;
    virtual void set_damping(std::size_t index, float damping) = 0;
};

class pei_debug_draw
{
public:
    virtual ~pei_debug_draw() = default;

    virtual void draw_line(const float3& start, const float3& end, const float3& color) = 0;
};

struct pei_world_desc
{
    float3 gravity;
    pei_debug_draw* debug_draw;
};

class pei_world
{
public:
    virtual ~pei_world() = default;

    virtual void add(
        pei_rigidbody* rigidbody,
        std::uint32_t collision_group,
        std::uint32_t collision_mask) = 0;
    virtual void add(pei_joint* joint) = 0;

    virtual void remove(pei_rigidbody* rigidbody) = 0;
    virtual void remove(pei_joint* joint) = 0;

    virtual void simulation(float time_step) = 0;

    virtual void debug() = 0;
};

class pei_plugin
{
public:
    virtual ~pei_plugin() = default;

    virtual pei_world* create_world(const pei_world_desc& desc) = 0;
    virtual void destroy_world(pei_world* world) = 0;

    virtual pei_collision_shape* create_collision_shape(const pei_collision_shape_desc& desc) = 0;
    virtual pei_collision_shape* create_collision_shape(
        const pei_collision_shape* const* child,
        const float4x4* offset,
        std::size_t size) = 0;
    virtual void destroy_collision_shape(pei_collision_shape* collision_shape) = 0;

    virtual pei_rigidbody* create_rigidbody(const pei_rigidbody_desc& desc) = 0;
    virtual void destroy_rigidbody(pei_rigidbody* rigidbody) = 0;

    virtual pei_joint* create_joint(const pei_joint_desc& desc) = 0;
    virtual void destroy_joint(pei_joint* joint) = 0;
};

using create_pei = pei_plugin* (*)();
using destroy_pei = void (*)(pei_plugin*);
} // namespace violet