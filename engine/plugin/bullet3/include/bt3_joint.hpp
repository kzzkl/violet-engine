#pragma once

#include "bt3_common.hpp"
#include "physics_interface.hpp"

namespace violet::physics::bullet3
{
class bt3_joint : public joint_interface
{
public:
    bt3_joint(const joint_desc& desc);
    virtual ~bt3_joint() = default;

    virtual void min_linear(const math::float3& linear) override;
    virtual void max_linear(const math::float3& linear) override;

    virtual void min_angular(const math::float3& angular) override;
    virtual void max_angular(const math::float3& angular) override;

    virtual void spring_enable(std::size_t i, bool enable) override;
    virtual void stiffness(std::size_t i, float stiffness) override;

    btGeneric6DofSpringConstraint* constraint() const noexcept { return m_constraint.get(); }

private:
    std::unique_ptr<btGeneric6DofSpringConstraint> m_constraint;
};
} // namespace violet::physics::bullet3