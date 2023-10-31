#pragma once

#include "bt3_common.hpp"
#include "bt3_rigidbody.hpp"
#include <vector>

namespace violet::bt3
{
class bt3_world : public pei_world
{
public:
    bt3_world(const pei_world_desc& desc);
    virtual ~bt3_world();

    virtual void add(
        pei_rigidbody* rigidbody,
        std::uint32_t collision_group,
        std::uint32_t collision_mask) override;
    virtual void add(pei_joint* joint) override;

    virtual void remove(pei_rigidbody* rigidbody) override;
    virtual void remove(pei_joint* joint) override;

    virtual void simulation(float time_step) override;

    virtual void debug() override {}

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_collision;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;

    std::unique_ptr<btDiscreteDynamicsWorld> m_world;

#ifndef NDEBUG
    class debug_draw;
    std::unique_ptr<debug_draw> m_debug_draw;
#endif
};
} // namespace violet::bt3