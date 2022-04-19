#pragma once

#include "bt3_common.hpp"
#include "physics_interface.hpp"

namespace ash::physics::bullet3
{
class bt3_joint : public joint_interface
{
public:
    bt3_joint(const joint_desc& desc);
    virtual ~bt3_joint() = default;

    btGeneric6DofSpringConstraint* constraint() const noexcept { return m_constraint.get(); }

private:
    std::unique_ptr<btGeneric6DofSpringConstraint> m_constraint;
};
} // namespace ash::physics::bullet3