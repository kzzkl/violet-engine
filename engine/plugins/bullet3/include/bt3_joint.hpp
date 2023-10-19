#pragma once

#include "bt3_common.hpp"
#include <memory>

namespace violet::bt3
{
class bt3_joint : public pei_joint
{
public:
    bt3_joint(const pei_joint_desc& desc);
    virtual ~bt3_joint() = default;

    virtual void set_min_linear(const float3& linear) override;
    virtual void set_max_linear(const float3& linear) override;

    virtual void set_min_angular(const float3& angular) override;
    virtual void set_max_angular(const float3& angular) override;

    virtual void set_spring_enable(std::size_t i, bool enable) override;
    virtual void set_stiffness(std::size_t i, float stiffness) override;

    btGeneric6DofSpringConstraint* get_constraint() const noexcept { return m_constraint.get(); }

private:
    std::unique_ptr<btGeneric6DofSpringConstraint> m_constraint;
};
} // namespace violet::bt3