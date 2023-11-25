#include "bt3_rigidbody.hpp"
#include "bt3_shape.hpp"
#include "bt3_world.hpp"
#include <iostream>

namespace violet::bt3
{
void bt3_motion_state::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
    centerOfMassWorldTrans.setFromOpenGLMatrix(&transform[0][0]);
}

void bt3_motion_state::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
    centerOfMassWorldTrans.getOpenGLMatrix(&transform[0][0]);
    updated_flag = true;
}

bt3_rigidbody::bt3_rigidbody(const pei_rigidbody_desc& desc)
{
    m_motion_state = std::make_unique<bt3_motion_state>();
    m_motion_state->transform = desc.initial_transform;
    m_motion_state->rigidbody = this;

    btCollisionShape* shape = static_cast<bt3_shape*>(desc.shape)->shape();

    btVector3 local_inertia(0, 0, 0);
    if (desc.mass != 0.0f)
        shape->calculateLocalInertia(desc.mass, local_inertia);

    btRigidBody::btRigidBodyConstructionInfo info(
        desc.mass,
        m_motion_state.get(),
        shape,
        local_inertia);

    info.m_linearDamping = desc.linear_damping;
    info.m_angularDamping = desc.angular_damping;
    info.m_restitution = desc.restitution;
    info.m_friction = desc.friction;
    info.m_additionalDamping = true;

    m_rigidbody = std::make_unique<btRigidBody>(info);

    if (desc.type == PEI_RIGIDBODY_TYPE_KINEMATIC)
    {
        m_rigidbody->setCollisionFlags(
            m_rigidbody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_rigidbody->setActivationState(DISABLE_DEACTIVATION);
    }
    else
    {
        set_activation_state(desc.activation_state);
    }
}

bt3_rigidbody::~bt3_rigidbody()
{
    if (m_world)
        m_world->remove(this);
}

void bt3_rigidbody::set_mass(float mass)
{
    btVector3 inertia;
    m_rigidbody->getCollisionShape()->calculateLocalInertia(mass, inertia);
    m_rigidbody->setMassProps(mass, inertia);
}

void bt3_rigidbody::set_damping(float linear_damping, float angular_damping)
{
    m_rigidbody->setDamping(linear_damping, angular_damping);
}

void bt3_rigidbody::set_restitution(float restitution)
{
    m_rigidbody->setRestitution(restitution);
}

void bt3_rigidbody::set_friction(float friction)
{
    m_rigidbody->setFriction(friction);
}

void bt3_rigidbody::set_shape(pei_collision_shape* shape)
{
    m_rigidbody->setCollisionShape(static_cast<bt3_shape*>(shape)->shape());
}

const float4x4& bt3_rigidbody::get_transform() const
{
    return m_motion_state->transform;
}

void bt3_rigidbody::set_transform(const float4x4& world)
{
    btTransform transform;
    transform.setFromOpenGLMatrix(&world[0][0]);
    m_motion_state->setWorldTransform(transform);
    m_rigidbody->setCenterOfMassTransform(transform);
}

void bt3_rigidbody::set_angular_velocity(const float3& velocity)
{
    m_rigidbody->setAngularVelocity(convert_vector(velocity));
}

void bt3_rigidbody::set_linear_velocity(const float3& velocity)
{
    m_rigidbody->setLinearVelocity(convert_vector(velocity));
}

void bt3_rigidbody::clear_forces()
{
    m_rigidbody->clearForces();
}

void bt3_rigidbody::set_activation_state(pei_rigidbody_activation_state state)
{
    switch (state)
    {
    case PEI_RIGIDBODY_ACTIVATION_STATE_ACTIVE:
        m_rigidbody->setActivationState(ACTIVE_TAG);
        break;
    case PEI_RIGIDBODY_ACTIVATION_STATE_DISABLE_DEACTIVATION:
        m_rigidbody->setActivationState(DISABLE_DEACTIVATION);
        break;
    case PEI_RIGIDBODY_ACTIVATION_STATE_DISABLE_SIMULATION:
        m_rigidbody->setActivationState(DISABLE_SIMULATION);
        break;
    default:
        break;
    }
}
} // namespace violet::bt3