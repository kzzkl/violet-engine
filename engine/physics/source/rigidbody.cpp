#include "rigidbody.hpp"
#include "log.hpp"

namespace ash::physics
{
/*
transform_reflect::transform_reflect(ecs::world* world) noexcept
 : m_world(world),
   m_entity(ecs::INVALID_ENTITY),
   m_offset(math::matrix_plain::identity()),
   m_offset_inverse(math::matrix_plain::identity())
{
}

const math::float4x4& transform_reflect::transform() const
{
 auto& node = m_world->component<scene::transform>(m_entity);

 math::float4x4_simd to_world = math::simd::load(node.world_matrix);
 math::float4x4_simd offset = math::simd::load(m_offset);

 math::simd::store(math::matrix_simd::mul(offset, to_world), m_transform);

 return m_transform;
}

void transform_reflect::transform(const math::float4x4& world)
{
 m_transform = world;

 math::float4x4_simd to_world = math::simd::load(world);
 math::float4x4_simd offset_inverse = math::simd::load(m_offset_inverse);

 auto& node = m_world->component<scene::transform>(m_entity);
 math::simd::store(math::matrix_simd::mul(offset_inverse, to_world), node.world_matrix);
}

void transform_reflect::offset(const math::float4x4& offset) noexcept
{
 m_offset = offset;

 math::float4x4_simd inverse = math::matrix_simd::inverse(math::simd::load(offset));
 math::simd::store(inverse, m_offset_inverse);
}

void transform_reflect::offset(const math::float4x4_simd& offset) noexcept
{
 math::simd::store(offset, m_offset);

 math::float4x4_simd inverse = math::matrix_simd::inverse(offset);
 math::simd::store(inverse, m_offset_inverse);
}*/

rigidbody::rigidbody()
    : m_mass(0.0f),
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