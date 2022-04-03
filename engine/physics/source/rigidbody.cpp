#include "rigidbody.hpp"

namespace ash::physics
{
rigidbody::rigidbody()
    : m_mass(0.0f),
      m_shape(nullptr),
      m_offset(math::matrix_plain::identity()),
      m_offset_inverse(math::matrix_plain::identity()),
      m_in_world(false)
{
}

void rigidbody::mass(float mass) noexcept
{
    m_mass = mass;

    if (interface != nullptr)
        interface->mass(mass);
}

void rigidbody::offset(const math::float4x4& offset) noexcept
{
    m_offset = offset;

    math::float4x4_simd temp = math::simd::load(m_offset);
    math::simd::store(math::matrix_simd::inverse(temp), m_offset_inverse);
}

void rigidbody::offset(const math::float4x4_simd& offset) noexcept
{
    math::simd::store(offset, m_offset);
    math::simd::store(math::matrix_simd::inverse(offset), m_offset_inverse);
}
} // namespace ash::physics