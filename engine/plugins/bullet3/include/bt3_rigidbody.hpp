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

    virtual void set_mass(float mass) override;
    virtual void set_damping(float linear_damping, float angular_damping) override;
    virtual void set_restitution(float restitution) override;
    virtual void set_friction(float friction) override;
    virtual void set_shape(phy_collision_shape* shape) override;

    virtual const mat4f& get_transform() const override;
    virtual void set_transform(const mat4f& world) override;

    virtual void set_angular_velocity(const vec3f& velocity) override;
    virtual void set_linear_velocity(const vec3f& velocity) override;

    virtual void clear_forces() override;

    virtual void set_activation_state(phy_rigidbody_activation_state state) override;

    virtual void set_updated_flag(bool flag) override;
    virtual bool get_updated_flag() const override;

    btRigidBody* get_rigidbody() const noexcept
    {
        return m_rigidbody.get();
    }

    void set_world(bt3_world* world)
    {
        m_world = world;
    }

private:
    std::unique_ptr<bt3_motion_state> m_motion_state;
    std::unique_ptr<btRigidBody> m_rigidbody;

    bt3_world* m_world;
};
} // namespace violet::bt3