#include "physics/joint.hpp"

namespace violet::physics
{
joint::joint()
    : m_relative_a(ecs::INVALID_ENTITY),
      m_relative_position_a{},
      m_relative_rotation_a{},
      m_relative_b(ecs::INVALID_ENTITY),
      m_relative_position_b{},
      m_relative_rotation_b{},
      m_min_linear{},
      m_max_linear{},
      m_min_angular{},
      m_max_angular{}
{
}

void joint::relative_rigidbody(
    ecs::entity rigidbody_a,
    const math::float3& relative_position_a,
    const math::float4& relative_rotation_a,
    ecs::entity rigidbody_b,
    const math::float3& relative_position_b,
    const math::float4& relative_rotation_b)
{
    VIOLET_ASSERT(m_interface == nullptr);

    m_relative_a = rigidbody_a;
    m_relative_position_a = relative_position_a;
    m_relative_rotation_a = relative_rotation_a;

    m_relative_b = rigidbody_b;
    m_relative_position_b = relative_position_b;
    m_relative_rotation_b = relative_rotation_b;
}

void joint::min_linear(const math::float3& min_linear)
{
    m_min_linear = min_linear;
    if (m_interface != nullptr)
        m_interface->min_linear(min_linear);
}

void joint::max_linear(const math::float3& max_linear)
{
    m_max_linear = max_linear;
    if (m_interface != nullptr)
        m_interface->max_linear(max_linear);
}

void joint::min_angular(const math::float3& min_angular)
{
    m_min_angular = min_angular;
    if (m_interface != nullptr)
        m_interface->min_angular(min_angular);
}

void joint::max_angular(const math::float3& max_angular)
{
    m_max_angular = max_angular;
    if (m_interface != nullptr)
        m_interface->max_angular(max_angular);
}

void joint::spring_enable(std::size_t i, bool enable)
{
    m_spring_enable[i] = enable;
    if (m_interface != nullptr)
        m_interface->spring_enable(i, enable);
}

void joint::stiffness(std::size_t i, float stiffness)
{
    m_stiffness[i] = stiffness;
    if (m_interface != nullptr)
        m_interface->stiffness(i, stiffness);
}
} // namespace violet::physics