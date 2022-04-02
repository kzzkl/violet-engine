#pragma once

#include "component.hpp"
#include "physics_exports.hpp"
#include "physics_interface.hpp"

namespace ash::physics
{
class PHYSICS_API rigidbody
{
public:
    rigidbody();

    void mass(float mass) noexcept;
    float mass() const noexcept { return m_mass; }

    void shape(collision_shape_interface* shape) noexcept { m_shape = shape; }
    collision_shape_interface* shape() const noexcept { return m_shape; }

    void in_world(bool in_world) noexcept { m_in_world = in_world; }
    bool in_world() const noexcept { return m_in_world; }

    std::unique_ptr<rigidbody_interface> interface;

private:
    float m_mass;
    collision_shape_interface* m_shape;

    bool m_in_world;
};
} // namespace ash::physics

namespace ash::ecs
{
template <>
struct component_trait<ash::physics::rigidbody>
{
    static constexpr std::size_t id = ash::uuid("308b46bf-898e-40fe-bd67-1d0d8e5a1aa3").hash();
};
} // namespace ash::ecs