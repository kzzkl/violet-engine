#include "bt3_joint.hpp"
#include "bt3_rigidbody.hpp"

namespace violet::bt3
{
bt3_joint::bt3_joint(const phy_joint_desc& desc)
{
    btMatrix3x3 rotate_a;
    rotate_a.setRotation(convert_quaternion(desc.source_rotation));

    btTransform frame_a;
    frame_a.setIdentity();
    frame_a.setOrigin(convert_vector(desc.source_position));
    frame_a.setBasis(rotate_a);

    btMatrix3x3 rotate_b;
    rotate_b.setRotation(convert_quaternion(desc.target_rotation));

    btTransform frame_b;
    frame_b.setIdentity();
    frame_b.setOrigin(convert_vector(desc.target_position));
    frame_b.setBasis(rotate_b);

    btRigidBody* rigidbody_a = static_cast<bt3_rigidbody*>(desc.source)->get_rigidbody();
    btRigidBody* rigidbody_b = static_cast<bt3_rigidbody*>(desc.target)->get_rigidbody();

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
        m_constraint->setDamping(i, desc.damping[i]);
    }
}

void bt3_joint::set_linear(const float3& min, const float3& max)
{
    m_constraint->setLinearLowerLimit(convert_vector(min));
    m_constraint->setLinearUpperLimit(convert_vector(max));
}

void bt3_joint::set_angular(const float3& min, const float3& max)
{
    m_constraint->setAngularLowerLimit(convert_vector(min));
    m_constraint->setAngularUpperLimit(convert_vector(max));
}

void bt3_joint::set_spring_enable(std::size_t index, bool enable)
{
    m_constraint->enableSpring(index, enable);
}

void bt3_joint::set_stiffness(std::size_t index, float stiffness)
{
    m_constraint->setStiffness(index, stiffness);
}

void bt3_joint::set_damping(std::size_t index, float damping)
{
    m_constraint->setDamping(index, damping);
}
} // namespace violet::bt3