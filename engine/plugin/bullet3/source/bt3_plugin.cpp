#include "bt3_joint.hpp"
#include "bt3_rigidbody.hpp"
#include "bt3_shape.hpp"
#include "bt3_world.hpp"

namespace violet::physics::bullet3
{

class bt3_factory : public factory
{
public:
    virtual world_interface* make_world(const world_desc& desc, debug_draw_interface* debug_draw)
        override
    {
        return new bt3_world(desc, debug_draw);
    }

    virtual collision_shape_interface* make_collision_shape(
        const collision_shape_desc& desc) override
    {
        return new bt3_shape(desc);
    }

    virtual collision_shape_interface* make_collision_shape(
        const collision_shape_interface* const* child,
        const math::float4x4* offset,
        std::size_t size) override
    {
        return new bt3_shape(child, offset, size);
    }

    virtual rigidbody_interface* make_rigidbody(const rigidbody_desc& desc) override
    {
        return new bt3_rigidbody(desc);
    }

    virtual joint_interface* make_joint(const joint_desc& desc) override
    {
        return new bt3_joint(desc);
    }
};
} // namespace violet::physics::bullet3

extern "C"
{
    PLUGIN_API violet::core::plugin_info get_plugin_info()
    {
        violet::core::plugin_info info = {};

        char name[] = "physics-bullet3";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API violet::physics::factory* make_factory()
    {
        return new violet::physics::bullet3::bt3_factory();
    }
}