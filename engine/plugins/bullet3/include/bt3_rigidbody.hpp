#pragma once

#include "bt3_common.hpp"
#include <memory>

namespace violet::bt3
{
class bt3_world;
class bt3_rigidbody;
class bt3_motion_state;

class bt3_rigidbody : public phy_rigidbody
{
public:
    bt3_rigidbody(const phy_rigidbody_desc& desc);
    virtual ~bt3_rigidbody();

    void set_mass(float mass) override;
    void set_damping(float linear_damping, float angular_damping) override;
    void set_restitution(float restitution) override;
    void set_friction(float friction) override;
    void set_shape(phy_collision_shape* shape) override;

    void set_angular_velocity(const vec3f& velocity) override;
    void set_linear_velocity(const vec3f& velocity) override;

    void clear_forces() override;

    void set_activation_state(phy_activation_state activation_state) override;
    void set_motion_state(phy_motion_state* motion_state) override;

    btRigidBody* get_rigidbody() const noexcept
    {
        return m_rigidbody.get();
    }

    std::uint32_t get_collision_group() const noexcept
    {
        return m_collision_group;
    }

    std::uint32_t get_collision_mask() const noexcept
    {
        return m_collision_mask;
    }

private:
    std::unique_ptr<btMotionState> m_motion_state;
    std::unique_ptr<btRigidBody> m_rigidbody;

    std::uint32_t m_collision_group;
    std::uint32_t m_collision_mask;
};
} // namespace violet::bt3