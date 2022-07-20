#include "physics/rigidbody.hpp"

namespace ash::physics
{
void rigidbody_transform_reflection::reflect(
    const math::float4x4& rigidbody_transform,
    const rigidbody& rigidbody,
    scene::transform& transform) const noexcept
{
    math::float4x4_simd to_world = math::simd::load(rigidbody_transform);
    math::float4x4_simd offset_inverse = math::simd::load(rigidbody.offset_inverse);
    transform.to_world(math::matrix_simd::mul(offset_inverse, to_world));
}

rigidbody::rigidbody()
{
    reflection = std::make_unique<rigidbody_transform_reflection>();
}
} // namespace ash::physics