#include "rigidbody.hpp"
#include "log.hpp"

namespace ash::physics
{
rigidbody::rigidbody()
    : m_type(rigidbody_type::DYNAMIC),
      m_mass(0.0f),
      m_shape(nullptr),
      m_in_world(false),
      m_collision_group(-1),
      m_collision_mask(-1),
      m_entity(ecs::INVALID_ENTITY),
      m_offset(math::matrix_plain::identity()),
      m_offset_inverse(math::matrix_plain::identity())
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
    math::float4x4_simd inverse = math::matrix_simd::inverse(math::simd::load(offset));
    math::simd::store(inverse, m_offset_inverse);
}

void rigidbody::offset(const math::float4x4_simd& offset) noexcept
{
    math::simd::store(offset, m_offset);
    math::float4x4_simd inverse = math::matrix_simd::inverse(offset);
    math::simd::store(inverse, m_offset_inverse);
}

void rigidbody::node(ecs::entity entity)
{
    m_entity = entity;
}
} // namespace ash::physics