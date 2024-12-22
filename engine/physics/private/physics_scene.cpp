#include "physics/physics_scene.hpp"

namespace violet
{
physics_scene::physics_scene(const vec3f& gravity, phy_debug_draw* debug, physics_context* context)
{
    phy_world_desc desc = {
        .gravity = gravity,
        .debug_draw = debug,
    };
    m_world = context->create_world(desc);
}

physics_scene::~physics_scene() {}

void physics_scene::add_rigidbody(phy_rigidbody* rigidbody)
{
    m_world->add(rigidbody);
}

void physics_scene::remove_rigidbody(phy_rigidbody* rigidbody)
{
    m_world->remove(rigidbody);
}

void physics_scene::add_joint(phy_joint* joint)
{
    m_world->add(joint);
}

void physics_scene::remove_joint(phy_joint* joint)
{
    m_world->remove(joint);
}

void physics_scene::simulation(float time_step)
{
    m_world->simulation(time_step);
}
} // namespace violet