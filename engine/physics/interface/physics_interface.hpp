#pragma once

#include "math.hpp"
#include "plugin_interface.hpp"

namespace ash::physics
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
};

struct rigidbody_desc
{
    rigidbody_type type;

    collision_shape_interface* shape;
    float mass;

    math::float4x4 world_matrix;
};

class rigidbody_interface
{
public:
    virtual ~rigidbody_interface() = default;

    virtual void mass(float mass) = 0;

    virtual void transform(const math::float4x4& world_matrix) = 0;
    virtual const math::float4x4& transform() const noexcept = 0;

    virtual bool updated() const noexcept = 0;
};

struct world_desc
{
    math::float3 gravity;
};

class world_interface
{
public:
    virtual ~world_interface() = default;

    virtual void add(rigidbody_interface* rigidbody) = 0;
    virtual void remove(rigidbody_interface* rigidbody) = 0;
    virtual void simulation(float time_step) = 0;
};

class factory_interface
{
public:
    virtual ~factory_interface() = default;

    virtual world_interface* make_world(const world_desc& desc) = 0;
    virtual collision_shape_interface* make_collision_shape(const collision_shape_desc& desc) = 0;
    virtual collision_shape_interface* make_collision_shape(
        const collision_shape_interface* const* child,
        const math::float4x4* offset,
        std::size_t size) = 0;
    virtual rigidbody_interface* make_rigidbody(const rigidbody_desc& desc) = 0;
};
using factory = factory_interface;

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

class context_interface
{
public:
    virtual factory_interface* factory() = 0;
    virtual void debug(debug_draw_interface* debug) {}
};
using context = context_interface;

using make_context = context_interface* (*)();
} // namespace ash::physics