#pragma once

#include "bt3_common.hpp"

namespace ash::physics::bullet3
{
class bt3_motion_state : public btMotionState
{
public:
    bt3_motion_state(const math::float4x4& world_matrix);

    virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const override;
    virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans) override;

    const math::float4x4& transform() noexcept
    {
        m_updated = false;
        return m_world_matrix;
    }

    bool updated() const noexcept { return m_updated; }

private:
    math::float4x4 m_world_matrix;
    bool m_updated;
};

class bt3_rigidbody : public rigidbody_interface
{
public:
    bt3_rigidbody(const rigidbody_desc& desc);

    virtual void mass(float mass) override;
    virtual void shape(collision_shape_interface* shape) override;

    virtual void transform(const math::float4x4& world_matrix) override;
    virtual const math::float4x4& transform() const noexcept override;

    btRigidBody* rigidbody() const noexcept { return m_rigidbody.get(); }

    virtual bool updated() const noexcept override { return m_motion_state->updated(); }

private:
    std::unique_ptr<bt3_motion_state> m_motion_state;
    std::unique_ptr<btRigidBody> m_rigidbody;
};
} // namespace ash::physics::bullet3