#include "joint.hpp"

namespace ash::physics
{
std::size_t joint::add_unit()
{
    m_units.push_back({});
    return m_units.size() - 1;
}

void joint::rigidbody(std::size_t index, rigidbody_handle rigidbody)
{
    m_units[index].rigidbody = rigidbody;
}

void joint::location(std::size_t index, const math::float3& location)
{
    m_units[index].location = location;
}

void joint::rotation(std::size_t index, const math::float4& rotation)
{
    m_units[index].rotation = rotation;
}

void joint::min_linear(std::size_t index, const math::float3& min_linear)
{
    m_units[index].min_linear = min_linear;
}

void joint::max_linear(std::size_t index, const math::float3& max_linear)
{
    m_units[index].max_linear = max_linear;
}

void joint::min_angular(std::size_t index, const math::float3& min_angular)
{
    m_units[index].min_angular = min_angular;
}

void joint::max_angular(std::size_t index, const math::float3& max_angular)
{
    m_units[index].max_angular = max_angular;
}

void joint::spring_translate_factor(std::size_t index, const math::float3& spring_translate_factor)
{
    m_units[index].spring_translate_factor = spring_translate_factor;
}

void joint::spring_rotate_factor(std::size_t index, const math::float3& spring_rotate_factor)
{
    m_units[index].spring_rotate_factor = spring_rotate_factor;
}
} // namespace ash::physics