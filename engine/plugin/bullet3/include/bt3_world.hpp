#pragma once

#include "bt3_common.hpp"

namespace ash::physics::bullet3
{
class bt3_world : public world_interface
{
public:
    bt3_world(const world_desc& desc);

    virtual void add(rigidbody_interface* rigidbody) override;
    virtual void remove(rigidbody_interface* rigidbody) override;
    virtual void simulation(float time_step) override;

    btDiscreteDynamicsWorld* world() const noexcept { return m_world.get(); }

private:
    std::unique_ptr<btDefaultCollisionConfiguration> m_collision;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;

    std::unique_ptr<btDiscreteDynamicsWorld> m_world;
};
} // namespace ash::physics::bullet3