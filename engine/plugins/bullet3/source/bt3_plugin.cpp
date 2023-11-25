#include "bt3_joint.hpp"
#include "bt3_shape.hpp"
#include "bt3_world.hpp"

namespace violet::bt3
{
class bt3_plugin : public pei_plugin
{
public:
    virtual pei_world* create_world(const pei_world_desc& desc) override
    {
        return new bt3_world(desc);
    }

    virtual void destroy_world(pei_world* world) override { delete world; }

    virtual pei_collision_shape* create_collision_shape(
        const pei_collision_shape_desc& desc) override
    {
        return new bt3_shape(desc);
    }

    virtual pei_collision_shape* create_collision_shape(
        const pei_collision_shape* const* child,
        const float4x4* offset,
        std::size_t size) override
    {
        return new bt3_shape(child, offset, size);
    }

    virtual void destroy_collision_shape(pei_collision_shape* collision_shape) override
    {
        delete collision_shape;
    }

    virtual pei_rigidbody* create_rigidbody(const pei_rigidbody_desc& desc) override
    {
        return new bt3_rigidbody(desc);
    }

    virtual void destroy_rigidbody(pei_rigidbody* rigidbody) override { delete rigidbody; }

    virtual pei_joint* create_joint(const pei_joint_desc& desc) override
    {
        return new bt3_joint(desc);
    }

    virtual void destroy_joint(pei_joint* joint) override { delete joint; }
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

    PLUGIN_API violet::pei_plugin* create_pei()
    {
        return new violet::bt3::bt3_plugin();
    }

    PLUGIN_API void destroy_pei(violet::pei_plugin* pei)
    {
        return delete pei;
    }
}