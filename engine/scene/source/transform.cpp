#include "scene/transform.hpp"
#include "log.hpp"

namespace ash::scene
{
transform::transform()
    : m_position{0.0f, 0.0f, 0.0f},
      m_rotation{0.0f, 0.0f, 0.0f, 1.0f},
      m_scale{1.0f, 1.0f, 1.0f},
      m_to_parent(math::matrix::identity()),
      m_to_world(math::matrix::identity()),
      m_in_scene(false),
      m_dirty(false),
      m_sync_count(0)
{
}

void transform::sync(const math::float4x4& parent_matrix)
{
    sync(math::simd::load(parent_matrix));
}

void transform::sync(const math::float4x4_simd& parent_matrix)
{
    math::float4_simd scale, rotation, position;
    math::matrix_simd::decompose(parent_matrix, scale, rotation, position);

    math::simd::store(parent_matrix, m_to_parent);
    math::simd::store(scale, m_scale);
    math::simd::store(rotation, m_rotation);
    math::simd::store(position, m_position);

    m_dirty = false;
    ++m_sync_count;
}

void transform::sync(const math::float4x4& parent_matrix, const math::float4x4& world_matrix)
{
    m_to_parent = parent_matrix;
    m_to_world = world_matrix;
    m_dirty = false;
    ++m_sync_count;
}

void transform::sync(
    const math::float4x4_simd& parent_matrix,
    const math::float4x4_simd& world_matrix)
{
    math::simd::store(parent_matrix, m_to_parent);
    math::simd::store(world_matrix, m_to_world);
    m_dirty = false;
    ++m_sync_count;
}
} // namespace ash::scene