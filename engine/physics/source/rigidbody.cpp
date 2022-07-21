#include "physics/rigidbody.hpp"
#include "physics/physics.hpp"

namespace ash::physics
{
void rigidbody_transform_reflection::reflect(
    const math::float4x4& rigidbody_transform,
    const rigidbody& rigidbody,
    scene::transform& transform) const noexcept
{
    math::float4x4_simd to_world = math::simd::load(rigidbody_transform);
    math::float4x4_simd offset_inverse = math::simd::load(rigidbody.offset_inverse());
    transform.to_world(math::matrix_simd::mul(offset_inverse, to_world));
}

rigidbody::rigidbody()
    : m_type(rigidbody_type::DYNAMIC),
      m_mass(0.0f),
      m_linear_damping(0.0f),
      m_angular_damping(0.0f),
      m_restitution(0.0f),
      m_friction(0.0f),
      m_shape(nullptr),
      m_collision_group(0),
      m_collision_mask(COLLISION_MASK_ALL),
      m_offset(math::matrix::identity()),
      m_offset_inverse(math::matrix::identity())
{
    m_reflection = std::make_unique<rigidbody_transform_reflection>();
}

void rigidbody::mass(float mass)
{
    m_mass = mass;
    if (m_interface != nullptr)
        m_interface->mass(mass);
}

void rigidbody::linear_damping(float linear_damping)
{
    m_linear_damping = linear_damping;
    if (m_interface != nullptr)
        m_interface->damping(linear_damping, m_angular_damping);
}

void rigidbody::angular_damping(float angular_damping)
{
    m_angular_damping = angular_damping;
    if (m_interface != nullptr)
        m_interface->damping(m_linear_damping, angular_damping);
}

void rigidbody::restitution(float restitution)
{
    m_restitution = restitution;
    if (m_interface != nullptr)
        m_interface->restitution(restitution);
}

void rigidbody::friction(float friction)
{
    m_friction = friction;
    if (m_interface != nullptr)
        m_interface->friction(friction);
}

void rigidbody::shape(collision_shape_interface* shape)
{
    m_shape = shape;
    if (m_interface != nullptr)
        m_interface->shape(shape);
}

void rigidbody::offset(const math::float4x4& offset) noexcept
{
    m_offset = offset;
    m_offset_inverse = math::matrix::inverse(offset);
}

void rigidbody::sync_world(const math::float4x4& rigidbody_transform, scene::transform& transform)
{
    m_reflection->reflect(rigidbody_transform, *this, transform);
}
} // namespace ash::physics