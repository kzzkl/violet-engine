#include "physics/physics_world.hpp"

namespace violet
{
physics_world::physics_world(const float3& gravity, pei_plugin* pei) : m_pei(pei)
{
    pei_world_desc desc = {};
    desc.gravity = gravity;
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
    }

    m_world->add(
        rigidbody->m_rigidbody,
        rigidbody->get_collision_group(),
        rigidbody->get_collision_mask());
}

void physics_world::simulation(float time_step)
{
    m_world->simulation(time_step);
}
} // namespace violet