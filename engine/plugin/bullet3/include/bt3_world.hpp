#pragma once

#include "bt3_common.hpp"
#include "bt3_rigidbody.hpp"
#include <vector>

namespace ash::physics::bullet3
{
class bt3_world : public world_interface
{
public:
    bt3_world(const world_desc& desc, debug_draw_interface* debug_draw);
    virtual ~bt3_world();

    virtual void add(
        rigidbody_interface* rigidbody,
        std::uint32_t collision_group,
        std::uint32_t collision_mask) override;
    virtual void add(joint_interface* joint) override;

    virtual void remove(rigidbody_interface* rigidbody) override;

    virtual void simulation(float time_step) override;

    virtual rigidbody_interface* updated_rigidbody() override
    {
        rigidbody_interface* result = nullptr;
        if (!m_updated_rigidbodies.empty())
        {
            result = m_updated_rigidbodies.back();
            m_updated_rigidbodies.pop_back();
        }
        return result;
    }

    void add_updated_rigidbody(bt3_rigidbody* rigidbody)
    {
        m_updated_rigidbodies.push_back(rigidbody);
    }

    btDiscreteDynamicsWorld* world() const noexcept { return m_world.get(); }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_collision;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;

    std::unique_ptr<btDiscreteDynamicsWorld> m_world;

    std::vector<bt3_rigidbody*> m_updated_rigidbodies;

#ifndef NDEBUG
    class bt3_debug_draw : public btIDebugDraw
    {
    public:
        bt3_debug_draw(debug_draw_interface* debug = nullptr) : m_debug(debug)
        {
            m_mode |= DebugDrawModes::DBG_DrawWireframe;
        }

        void debug(debug_draw_interface* debug) { m_debug = debug; }

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
        debug_draw_interface* m_debug;
    };

    std::unique_ptr<bt3_debug_draw> m_debug_draw;
#endif
};
} // namespace ash::physics::bullet3