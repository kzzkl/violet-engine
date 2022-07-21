#include "bt3_joint.hpp"
#include "bt3_rigidbody.hpp"

namespace ash::physics::bullet3
{
bt3_joint::bt3_joint(const joint_desc& desc)
{
    btMatrix3x3 rotate_a;
    rotate_a.setRotation(convert_quaternion(desc.relative_rotation_a));

    btTransform frame_a;
    frame_a.setIdentity();
    frame_a.setOrigin(convert_vector(desc.relative_position_a));
    frame_a.setBasis(rotate_a);

    btMatrix3x3 rotate_b;
    rotate_b.setRotation(convert_quaternion(desc.relative_rotation_b));

    btTransform frame_b;
    frame_b.setIdentity();
    frame_b.setOrigin(convert_vector(desc.relative_position_b));
    frame_b.setBasis(rotate_b);

    btRigidBody* rigidbody_a = static_cast<bt3_rigidbody*>(desc.rigidbody_a)->rigidbody();
    btRigidBody* rigidbody_b = static_cast<bt3_rigidbody*>(desc.rigidbody_b)->rigidbody();

    m_constraint = std::make_unique<btGeneric6DofSpringConstraint>(
        *rigidbody_a,
        *rigidbody_b,
        frame_a,
        frame_b,
        true);
    m_constraint->setLinearLowerLimit(convert_vector(desc.min_linear));
    m_constraint->setLinearUpperLimit(convert_vector(desc.max_linear));

    m_constraint->setAngularLowerLimit(convert_vector(desc.min_angular));
    m_constraint->setAngularUpperLimit(convert_vector(desc.max_angular));

    for (int i = 0; i < 6; ++i)
    {
        m_constraint->enableSpring(i, desc.spring_enable[i]);
        m_constraint->setStiffness(i, desc.stiffness[i]);
    }
}

void bt3_joint::min_linear(const math::float3& linear)
{
    m_constraint->setLinearLowerLimit(convert_vector(linear));
}

void bt3_joint::max_linear(const math::float3& linear)
{
    m_constraint->setLinearUpperLimit(convert_vector(linear));
}

void bt3_joint::min_angular(const math::float3& angular)
{
    m_constraint->setAngularLowerLimit(convert_vector(angular));
}

void bt3_joint::max_angular(const math::float3& angular)
{
    m_constraint->setAngularUpperLimit(convert_vector(angular));
}

void bt3_joint::spring_enable(std::size_t i, bool enable)
{
    m_constraint->enableSpring(i, enable);
}

void bt3_joint::stiffness(std::size_t i, float stiffness)
{
    m_constraint->setStiffness(i, stiffness);
}
} // namespace ash::physics::bullet3