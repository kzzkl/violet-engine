#include "bt3_world.hpp"
#include "bt3_joint.hpp"
#include "bt3_rigidbody.hpp"

namespace violet::physics::bullet3
{
bt3_world::bt3_world(const world_desc& desc, debug_draw_interface* debug_draw)
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

#ifndef NDEBUG
    if (debug_draw)
    {
        m_debug_draw = std::make_unique<bt3_debug_draw>(debug_draw);
        m_world->setDebugDrawer(m_debug_draw.get());
    }
#endif
}

bt3_world::~bt3_world()
{
    for (std::size_t i = 0; i < m_world->getNumCollisionObjects(); ++i)
    {
        btCollisionObject* object = m_world->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(object);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        m_world->removeCollisionObject(object);
        delete object;
    }

    m_world = nullptr;
    m_solver = nullptr;
    m_broadphase = nullptr;
    m_dispatcher = nullptr;
    m_collision = nullptr;

#ifndef NDEBUG
    m_debug_draw = nullptr;
#endif
}

void bt3_world::add(
    rigidbody_interface* rigidbody,
    std::uint32_t collision_group,
    std::uint32_t collision_mask)
{
    auto r = static_cast<bt3_rigidbody*>(rigidbody);
    m_world->addRigidBody(
        r->rigidbody(),
        static_cast<int>(collision_group),
        static_cast<int>(collision_mask));

    r->world(this);
}

void bt3_world::add(joint_interface* joint)
{
    m_world->addConstraint(static_cast<bt3_joint*>(joint)->constraint());
}

void bt3_world::remove(rigidbody_interface* rigidbody)
{
    m_world->removeCollisionObject(static_cast<bt3_rigidbody*>(rigidbody)->rigidbody());
}

void bt3_world::simulation(float time_step)
{
    m_updated_rigidbodies.clear();
    m_world->stepSimulation(time_step);

#ifndef NDEBUG
    if (m_world->getDebugDrawer())
        m_world->debugDrawWorld();
#endif
}
} // namespace violet::physics::bullet3