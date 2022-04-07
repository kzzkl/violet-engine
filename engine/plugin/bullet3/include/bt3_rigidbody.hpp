#pragma once

#include "bt3_common.hpp"

namespace ash::physics::bullet3
{
class bt3_motion_state : public btMotionState
{
public:
    bt3_motion_state(transform_reflect_interface* reflect);

    virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const override;
    virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans) override;

protected:
    transform_reflect_interface* m_reflect;
};

class bt3_rigidbody : public rigidbody_interface
{
public:
    bt3_rigidbody(const rigidbody_desc& desc);

    virtual void mass(float mass) override;
    virtual void shape(collision_shape_interface* shape) override;

    btRigidBody* rigidbody() const noexcept { return m_rigidbody.get(); }

private:
    std::unique_ptr<bt3_motion_state> m_motion_state;
    std::unique_ptr<btRigidBody> m_rigidbody;
};
} // namespace ash::physics::bullet3