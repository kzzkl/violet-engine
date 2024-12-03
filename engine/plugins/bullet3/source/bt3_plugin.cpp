#include "bt3_joint.hpp"
#include "bt3_shape.hpp"
#include "bt3_world.hpp"
#include "core/plugin_interface.hpp"

namespace violet::bt3
{
class bt3_plugin : public phy_plugin
{
public:
    virtual phy_world* create_world(const phy_world_desc& desc) override
    {
        return new bt3_world(desc);
    }

    virtual void destroy_world(phy_world* world) override
    {
        delete world;
    }

    virtual phy_collision_shape* create_collision_shape(
        const phy_collision_shape_desc& desc) override
    {
        return new bt3_shape(desc);
    }

    virtual phy_collision_shape* create_collision_shape(
        const phy_collision_shape* const* child,
        const mat4f* offset,
        std::size_t size) override
    {
        return new bt3_shape(child, offset, size);
    }

    virtual void destroy_collision_shape(phy_collision_shape* collision_shape) override
    {
        delete collision_shape;
    }

    virtual phy_rigidbody* create_rigidbody(const phy_rigidbody_desc& desc) override
    {
        return new bt3_rigidbody(desc);
    }

    virtual void destroy_rigidbody(phy_rigidbody* rigidbody) override
    {
        delete rigidbody;
    }

    virtual phy_joint* create_joint(const phy_joint_desc& desc) override
    {
        return new bt3_joint(desc);
    }

    virtual void destroy_joint(phy_joint* joint) override
    {
        delete joint;
    }
};
} // namespace violet::bt3

extern "C"
{
    PLUGIN_API violet::plugin_info get_plugin_info()
    {
        violet::plugin_info info = {};

        char name[] = "physics-bullet3";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API violet::phy_plugin* phy_create_plugin()
    {
        return new violet::bt3::bt3_plugin();
    }

    PLUGIN_API void phy_destroy_plugin(violet::phy_plugin* plugin)
    {
        return delete plugin;
    }
}