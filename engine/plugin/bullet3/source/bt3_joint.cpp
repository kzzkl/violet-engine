#include "bt3_joint.hpp"
#include "bt3_rigidbody.hpp"

namespace ash::physics::bullet3
{
bt3_joint::bt3_joint(const joint_desc& desc)
{
    btMatrix3x3 rotate;
    rotate.setRotation(convert_quaternion(desc.rotation));

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(convert_vector(desc.location));
    transform.setBasis(rotate);

    btRigidBody* rigidbody_a = static_cast<bt3_rigidbody*>(desc.rigidbody_a)->rigidbody();
    btRigidBody* rigidbody_b = static_cast<bt3_rigidbody*>(desc.rigidbody_b)->rigidbody();

    btTransform inverse_a = rigidbody_a->getWorldTransform().inverse() * transform;
    btTransform inverse_b = rigidbody_b->getWorldTransform().inverse() * transform;

    m_constraint = std::make_unique<btGeneric6DofSpringConstraint>(
        *rigidbody_a,
        *rigidbody_b,
        inverse_a,
        inverse_b,
        true);
    m_constraint->setLinearLowerLimit(convert_vector(desc.min_linear));
    m_constraint->setLinearUpperLimit(convert_vector(desc.max_linear));

    m_constraint->setAngularLowerLimit(convert_vector(desc.min_angular));
    m_constraint->setAngularUpperLimit(convert_vector(desc.max_angular));

    for (int i = 0; i < 3; ++i)
    {
        if (desc.spring_translate_factor[i] != 0)
        {
            m_constraint->enableSpring(i, true);
            m_constraint->setStiffness(i, desc.spring_translate_factor[i]);
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        if (desc.spring_rotate_factor[i] != 0)
        {
            m_constraint->enableSpring(i + 3, true);
            m_constraint->setStiffness(i + 3, desc.spring_rotate_factor[i]);
        }
    }
}
} // namespace ash::physics::bullet3