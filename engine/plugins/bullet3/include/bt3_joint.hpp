#pragma once

#include "bt3_common.hpp"
#include <memory>

namespace violet::bt3
{
class bt3_joint : public phy_joint
{
public:
    bt3_joint(const phy_joint_desc& desc);
    virtual ~bt3_joint() = default;

    virtual void set_linear(const vec3f& min, const vec3f& max) override;
    virtual void set_angular(const vec3f& min, const vec3f& max) override;

    virtual void set_spring_enable(std::size_t index, bool enable) override;
    virtual void set_stiffness(std::size_t index, float stiffness) override;
    virtual void set_damping(std::size_t index, float damping) override;

    btGeneric6DofSpringConstraint* get_constraint() const noexcept
    {
        return m_constraint.get();
    }

private:
    std::unique_ptr<btGeneric6DofSpringConstraint> m_constraint;
};
} // namespace violet::bt3