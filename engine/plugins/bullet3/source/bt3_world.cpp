#include "bt3_world.hpp"
#include "bt3_joint.hpp"
#include "bt3_rigidbody.hpp"

namespace violet::bt3
{
#ifndef NDEBUG
class bt3_world::debug_draw : public btIDebugDraw
{
public:
    debug_draw(pei_debug_draw* debug = nullptr) : m_debug(debug)
    {
        m_mode |= DebugDrawModes::DBG_DrawWireframe;
    }

    void debug(pei_debug_draw* debug) { m_debug = debug; }

    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
        override
    {
        m_debug->draw_line(convert_vector(from), convert_vector(to), convert_vector(color));
    }

    virtual void drawContactPoint(
        const btVector3& PointOnB,
        const btVector3& normalOnB,
        btScalar distance,
        int lifeTime,
        const btVector3& color) override
    {
    }
    virtual void reportErrorWarning(const char* warningString) override {}
    virtual void draw3dText(const btVector3& location, const char* textString) override {}
    virtual void setDebugMode(int debugMode) override { m_mode = debugMode; }
    virtual int getDebugMode() const override { return m_mode; }

private:
    int m_mode;
    pei_debug_draw* m_debug;
};
#endif

bt3_world::bt3_world(const pei_world_desc& desc)
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
    if (desc.debug_draw)
    {
        m_debug_draw = std::make_unique<debug_draw>(desc.debug_draw);
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
    pei_rigidbody* rigidbody,
    std::uint32_t collision_group,
    std::uint32_t collision_mask)
{
    auto r = static_cast<bt3_rigidbody*>(rigidbody);
    m_world->addRigidBody(
        r->get_rigidbody(),
        static_cast<int>(collision_group),
        static_cast<int>(collision_mask));

    r->set_world(this);
}

void bt3_world::add(pei_joint* joint)
{
    m_world->addConstraint(static_cast<bt3_joint*>(joint)->get_constraint());
}

void bt3_world::remove(pei_rigidbody* rigidbody)
{
    m_world->removeCollisionObject(static_cast<bt3_rigidbody*>(rigidbody)->get_rigidbody());
}

void bt3_world::remove(pei_joint* joint)
{
    m_world->removeConstraint(static_cast<bt3_joint*>(joint)->get_constraint());
}

void bt3_world::simulation(float time_step)
{
    m_world->stepSimulation(time_step, 10, 1.0f / 120.0f);

#ifndef NDEBUG
    if (m_world->getDebugDrawer())
        m_world->debugDrawWorld();
#endif
}
} // namespace violet::bt3