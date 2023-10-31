#include "physics/physics_world.hpp"

namespace violet
{
physics_world::physics_world(const float3& gravity, pei_debug_draw* debug, pei_plugin* pei)
    : m_pei(pei)
{
    pei_world_desc desc = {};
    desc.gravity = gravity;
    desc.debug_draw = debug;
    m_world = pei->create_world(desc);
}

physics_world::~physics_world()
{
    m_pei->destroy_world(m_world);
}

void physics_world::add(component_ptr<rigidbody> rigidbody)
{
    rigidbody->set_world(m_world);

    m_world->add(
        rigidbody->get_rigidbody(),
        rigidbody->get_collision_group(),
        rigidbody->get_collision_mask());

    auto joints = rigidbody->get_joints();

    for (pei_joint* joint : joints)
        m_world->add(joint);
}

void physics_world::simulation(float time_step)
{
    m_world->simulation(time_step);
}
} // namespace violet