#include "bt3_world.hpp"
#include "bt3_rigidbody.hpp"

namespace ash::physics::bullet3
{
bt3_world::bt3_world(const world_desc& desc)
{
    m_collision = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collision.get());
    m_broadphase = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    m_world = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(),
        m_broadphase.get(),
        m_solver.get(),
        m_collision.get());
    m_world->setGravity(btVector3(desc.gravity[0], desc.gravity[1], desc.gravity[2]));
}

void bt3_world::add(rigidbody_interface* rigidbody)
{
    m_world->addRigidBody(static_cast<bt3_rigidbody*>(rigidbody)->rigidbody());
}

void bt3_world::remove(rigidbody_interface* rigidbody)
{
    m_world->removeRigidBody(static_cast<bt3_rigidbody*>(rigidbody)->rigidbody());
}

void bt3_world::simulation(float time_step)
{
    m_world->stepSimulation(time_step);

    if (m_world->getDebugDrawer())
        m_world->debugDrawWorld();
}
} // namespace ash::physics::bullet3