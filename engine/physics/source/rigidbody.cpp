#include "rigidbody.hpp"
#include "log.hpp"

namespace ash::physics
{

transform_reflect::transform_reflect() noexcept
    : m_node(nullptr),
      m_offset(math::matrix_plain::identity()),
      m_offset_inverse(math::matrix_plain::identity())
{
}

const math::float4x4& transform_reflect::transform() const
{
    math::float4x4_simd to_world = math::simd::load(m_node->transform->world_matrix);
    math::float4x4_simd offset = math::simd::load(m_offset);

    math::simd::store(math::matrix_simd::mul(offset, to_world), m_transform);

    return m_transform;
}

void transform_reflect::transform(const math::float4x4& world)
{
    m_transform = world;

    math::float4x4_simd to_world = math::simd::load(world);
    math::float4x4_simd offset_inverse = math::simd::load(m_offset_inverse);

    math::simd::store(
        math::matrix_simd::mul(offset_inverse, to_world),
        m_node->transform->world_matrix);
    //m_node->dirty = true;
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
}

rigidbody::rigidbody()
    : m_mass(0.0f),
      m_shape(nullptr),
      m_in_world(false),
      m_reflect(std::make_unique<transform_reflect>()),
      m_collision_group(-1),
      m_collision_mask(-1)
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
    m_reflect->offset(offset);
}

void rigidbody::offset(const math::float4x4_simd& offset) noexcept
{
    m_reflect->offset(offset);
}

void rigidbody::node(ash::scene::transform_node* node)
{
    m_reflect->node(node);
}
} // namespace ash::physics