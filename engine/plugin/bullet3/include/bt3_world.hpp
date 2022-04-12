#pragma once

#include "bt3_common.hpp"
#include "bt3_rigidbody.hpp"
#include <vector>

namespace ash::physics::bullet3
{
class bt3_world : public world_interface
{
public:
    bt3_world(const world_desc& desc);

    virtual void add(
        rigidbody_interface* rigidbody,
        std::uint32_t collision_group,
        std::uint32_t collision_mask) override;
    virtual void add(joint_interface* joint) override;

    virtual void remove(rigidbody_interface* rigidbody) override;

    virtual void simulation(float time_step) override;
    virtual void debug() override;

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
};
} // namespace ash::physics::bullet3