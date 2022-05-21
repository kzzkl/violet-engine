#pragma once

#include "bt3_common.hpp"

namespace ash::physics::bullet3
{
class bt3_world;
class bt3_rigidbody;
class bt3_motion_state : public btMotionState
{
public:
    virtual void getWorldTransform(btTransform& centerOfMassWorldTrans) const override;
    virtual void setWorldTransform(const btTransform& centerOfMassWorldTrans) override;

    bt3_rigidbody* rigidbody;
    bt3_world* world;
};

class bt3_rigidbody : public rigidbody_interface
{
public:
    bt3_rigidbody(const rigidbody_desc& desc);
    virtual ~bt3_rigidbody();

    virtual void mass(float mass) override;
    virtual void shape(collision_shape_interface* shape) override;

    virtual const math::float4x4& transform() const override;
    virtual void transform(const math::float4x4& world) override;

    virtual void angular_velocity(const math::float3& velocity) override;
    virtual void linear_velocity(const math::float3& velocity) override;

    virtual void clear_forces() override;

    btRigidBody* rigidbody() const noexcept { return m_rigidbody.get(); }
    void world(bt3_world* world) { m_motion_state->world = world; }

private:
    std::unique_ptr<bt3_motion_state> m_motion_state;
    std::unique_ptr<btRigidBody> m_rigidbody;

    math::float4x4 m_transform;
};
} // namespace ash::physics::bullet3