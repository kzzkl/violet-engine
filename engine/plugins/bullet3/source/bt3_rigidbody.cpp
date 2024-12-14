#include "bt3_rigidbody.hpp"
#include "bt3_shape.hpp"

namespace violet::bt3
{
class bt3_motion_state : public btMotionState
{
public:
    void getWorldTransform(btTransform& world_transform) const override
    {
        world_transform.setFromOpenGLMatrix(&transform[0][0]);
    }

    void setWorldTransform(const btTransform& world_transform) override
    {
        world_transform.getOpenGLMatrix(&transform[0][0]);
    }

    mat4f transform;
};

class bt3_motion_state_kinematic : public bt3_motion_state
{
public:
    void setWorldTransform(const btTransform& world_transform) override {}
};

class bt3_motion_state_custom : public btMotionState
{
public:
    bt3_motion_state_custom(phy_motion_state* motion_state)
        : m_motion_state(motion_state)
    {
    }

    void getWorldTransform(btTransform& world_transform) const override
    {
        const auto& transform = m_motion_state->get_transform();
        world_transform.setFromOpenGLMatrix(&transform[0][0]);
    }

    void setWorldTransform(const btTransform& world_transform) override
    {
        mat4f transform;
        world_transform.getOpenGLMatrix(&transform[0][0]);
        m_motion_state->set_transform(transform);
    }

private:
    phy_motion_state* m_motion_state;
};

bt3_rigidbody::bt3_rigidbody(const phy_rigidbody_desc& desc)
    : m_collision_group(desc.collision_group),
      m_collision_mask(desc.collision_mask)
{
    if (desc.type == PHY_RIGIDBODY_TYPE_KINEMATIC)
    {
        m_motion_state = std::make_unique<bt3_motion_state_kinematic>();
    }
    else
    {
        m_motion_state = std::make_unique<bt3_motion_state>();
    }

    btTransform initial_transform;
    initial_transform.setFromOpenGLMatrix(&desc.initial_transform[0][0]);
    m_motion_state->setWorldTransform(initial_transform);

    btCollisionShape* shape = static_cast<bt3_shape*>(desc.shape)->shape();

    btVector3 local_inertia(0, 0, 0);
    if (desc.mass != 0.0f)
    {
        shape->calculateLocalInertia(desc.mass, local_inertia);
    }

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

    if (desc.type == PHY_RIGIDBODY_TYPE_KINEMATIC)
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

bt3_rigidbody::~bt3_rigidbody() {}

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

void bt3_rigidbody::set_shape(phy_collision_shape* shape)
{
    m_rigidbody->setCollisionShape(static_cast<bt3_shape*>(shape)->shape());
}

void bt3_rigidbody::set_angular_velocity(const vec3f& velocity)
{
    m_rigidbody->setAngularVelocity(convert_vector(velocity));
}

void bt3_rigidbody::set_linear_velocity(const vec3f& velocity)
{
    m_rigidbody->setLinearVelocity(convert_vector(velocity));
}

void bt3_rigidbody::clear_forces()
{
    m_rigidbody->clearForces();
}

void bt3_rigidbody::set_activation_state(phy_activation_state activation_state)
{
    switch (activation_state)
    {
    case PHY_ACTIVATION_STATE_ACTIVE:
        m_rigidbody->setActivationState(ACTIVE_TAG);
        break;
    case PHY_ACTIVATION_STATE_DISABLE_DEACTIVATION:
        m_rigidbody->setActivationState(DISABLE_DEACTIVATION);
        break;
    case PHY_ACTIVATION_STATE_DISABLE_SIMULATION:
        m_rigidbody->setActivationState(DISABLE_SIMULATION);
        break;
    default:
        break;
    }
}

void bt3_rigidbody::set_motion_state(phy_motion_state* motion_state)
{
    btTransform transform;
    m_motion_state->getWorldTransform(transform);
    m_motion_state = std::make_unique<bt3_motion_state_custom>(motion_state);
    m_motion_state->setWorldTransform(transform);
    m_rigidbody->setMotionState(m_motion_state.get());
}
} // namespace violet::bt3