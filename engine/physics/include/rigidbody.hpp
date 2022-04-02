#pragma once

#include "component.hpp"
#include "physics_interface.hpp"

namespace ash::physics
{
struct rigidbody
{
    std::unique_ptr<rigidbody_interface> interface;
    bool in_world{false};

    collision_shape_interface* shape;
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