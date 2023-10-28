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
    if (rigidbody->m_rigidbody == nullptr)
    {
        rigidbody->m_rigidbody = m_pei->create_rigidbody(rigidbody->m_desc);
        rigidbody->m_pei = m_pei;

        for (auto& joint : rigidbody->m_joints)
        {
            if (joint->m_target->m_rigidbody == nullptr)
            {
                joint->m_target->m_rigidbody = m_pei->create_rigidbody(joint->m_target->m_desc);
                joint->m_target->m_pei = m_pei;

                joint->m_joint = m_pei->create_joint(joint->m_desc);
            }
        }
    }

    m_world->add(
        rigidbody->m_rigidbody,
        rigidbody->get_collision_group(),
        rigidbody->get_collision_mask());

    for (auto& joint : rigidbody->m_joints)
        m_world->add(joint->m_joint);
}

void physics_world::simulation(float time_step)
{
    m_world->simulation(time_step);
}
} // namespace violet