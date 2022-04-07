#include "bt3_rigidbody.hpp"
#include "bt3_shape.hpp"
#include <iostream>

namespace ash::physics::bullet3
{
bt3_motion_state::bt3_motion_state(transform_reflect_interface* reflect) : m_reflect(reflect)
{
}

void bt3_motion_state::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
    centerOfMassWorldTrans.setFromOpenGLMatrix(&m_reflect->transform()[0][0]);
}

void bt3_motion_state::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
    math::float4x4 world_matrix;
    centerOfMassWorldTrans.getOpenGLMatrix(&world_matrix[0][0]);
    m_reflect->transform(world_matrix);
}

bt3_rigidbody::bt3_rigidbody(const rigidbody_desc& desc)
{
    m_motion_state = std::make_unique<bt3_motion_state>(desc.reflect);

    btCollisionShape* shape = static_cast<bt3_shape*>(desc.shape)->shape();

    btVector3 local_inertia(0, 0, 0);
    if (desc.mass != 0.0f)
        shape->calculateLocalInertia(desc.mass, local_inertia);

    btRigidBody::btRigidBodyConstructionInfo info(
        desc.mass,
        m_motion_state.get(),
        shape,
        local_inertia);

    info.m_linearDamping = desc.linear_dimmer;
    info.m_angularDamping = desc.angular_dimmer;
    info.m_restitution = desc.restitution;
    info.m_friction = desc.friction;
    info.m_additionalDamping = true;

    m_rigidbody = std::make_unique<btRigidBody>(info);
}

void bt3_rigidbody::mass(float mass)
{
    btVector3 inertia;
    m_rigidbody->getCollisionShape()->calculateLocalInertia(mass, inertia);
    m_rigidbody->setMassProps(mass, inertia);
}

void bt3_rigidbody::shape(collision_shape_interface* shape)
{
    m_rigidbody->setCollisionShape(static_cast<bt3_shape*>(shape)->shape());
}
} // namespace ash::physics::bullet3