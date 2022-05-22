#include "bt3_rigidbody.hpp"
#include "bt3_shape.hpp"
#include "bt3_world.hpp"

namespace ash::physics::bullet3
{
void bt3_motion_state::getWorldTransform(btTransform& centerOfMassWorldTrans) const
{
    centerOfMassWorldTrans.setFromOpenGLMatrix(&rigidbody->transform()[0][0]);
}

void bt3_motion_state::setWorldTransform(const btTransform& centerOfMassWorldTrans)
{
    math::float4x4 world_matrix;
    centerOfMassWorldTrans.getOpenGLMatrix(&world_matrix[0][0]);
    rigidbody->transform(world_matrix);

    world->add_updated_rigidbody(rigidbody);
}

bt3_rigidbody::bt3_rigidbody(const rigidbody_desc& desc) : m_transform(desc.initial_transform)
{
    m_motion_state = std::make_unique<bt3_motion_state>();
    m_motion_state->rigidbody = this;
    m_motion_state->world = nullptr;

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

    if (desc.type == rigidbody_type::KINEMATIC)
    {
        m_rigidbody->setCollisionFlags(
            m_rigidbody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_rigidbody->setActivationState(DISABLE_DEACTIVATION);
    }
}

bt3_rigidbody::~bt3_rigidbody()
{
    if (m_motion_state->world)
        m_motion_state->world->remove(this);
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

const math::float4x4& bt3_rigidbody::transform() const
{
    return m_transform;
}

void bt3_rigidbody::transform(const math::float4x4& world)
{
    m_transform = world;
}

void bt3_rigidbody::angular_velocity(const math::float3& velocity)
{
    m_rigidbody->setAngularVelocity(convert_vector(velocity));
}

void bt3_rigidbody::linear_velocity(const math::float3& velocity)
{
    m_rigidbody->setLinearVelocity(convert_vector(velocity));
}

void bt3_rigidbody::clear_forces()
{
    m_rigidbody->clearForces();
}
} // namespace ash::physics::bullet3