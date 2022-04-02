#include "rigidbody.hpp"

namespace ash::physics
{
rigidbody::rigidbody() : m_mass(0.0f), m_shape(nullptr), m_in_world(false)
{
}

void rigidbody::mass(float mass) noexcept
{
    m_mass = mass;

    if (interface != nullptr)
        interface->mass(mass);
}
} // namespace ash::physics