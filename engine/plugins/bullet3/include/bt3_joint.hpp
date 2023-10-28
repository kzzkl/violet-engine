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

    virtual void set_linear(const float3& min, const float3& max) override;
    virtual void set_angular(const float3& min, const float3& max) override;

    virtual void set_spring_enable(std::size_t i, bool enable) override;
    virtual void set_stiffness(std::size_t i, float stiffness) override;

    btGeneric6DofSpringConstraint* get_constraint() const noexcept { return m_constraint.get(); }

private:
    std::unique_ptr<btGeneric6DofSpringConstraint> m_constraint;
};
} // namespace violet::bt3