#include "bt3_rigidbody.hpp"
#include "bt3_shape.hpp"

namespace ash::physics::bullet3
{
bt3_motion_state::bt3_motion_state(const math::float4x4& world_matrix)
    : m_world_matrix(world_matrix)
{
}

void bt3_motion_state::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
    centerOfMassWorldTrans.setFromOpenGLMatrix(&m_world_matrix[0][0]);
}

void bt3_motion_state::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
    m_updated = true;
    centerOfMassWorldTrans.getOpenGLMatrix(&m_world_matrix[0][0]);
}

bt3_rigidbody::bt3_rigidbody(const rigidbody_desc& desc)
{
    m_motion_state = std::make_unique<bt3_motion_state>(desc.world_matrix);

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

void bt3_rigidbody::transform(const math::float4x4& world_matrix)
{
    btTransform transform;
    transform.setFromOpenGLMatrix(&world_matrix[0][0]);
    m_rigidbody->getMotionState()->setWorldTransform(transform);
}

const math::float4x4& bt3_rigidbody::transform() const noexcept
{
    return m_motion_state->transform();
}
} // namespace ash::physics::bullet3