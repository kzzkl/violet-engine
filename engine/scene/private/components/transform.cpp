#include "components/transform.hpp"

namespace violet
{
transform::transform() noexcept
    : m_position{0.0f, 0.0f, 0.0f},
      m_rotation{0.0f, 0.0f, 0.0f, 1.0f},
      m_scale{1.0f, 1.0f, 1.0f},
      m_local_matrix(matrix::identity()),
      m_world_matrix(matrix::identity()),
      m_dirty_flag(DIRTY_FLAG_NONE),
      m_update_count(0)
{
}

void transform::set_position(float x, float y, float z) noexcept
{
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
    m_dirty_flag = DIRTY_FLAG_ALL;
}

void transform::set_position(const float3& position) noexcept
{
    m_position = position;
    m_dirty_flag = DIRTY_FLAG_ALL;
}

void transform::set_position(float4_simd position) noexcept
{
    simd::store(position, m_position);
    m_dirty_flag = DIRTY_FLAG_ALL;
}

const float3& transform::get_position() const noexcept
{
    return m_position;
}

void transform::set_rotation(const float4& quaternion) noexcept
{
    m_rotation = quaternion;
    m_dirty_flag = DIRTY_FLAG_ALL;
}

void transform::set_rotation(float4_simd quaternion) noexcept
{
    simd::store(quaternion, m_rotation);
    m_dirty_flag = DIRTY_FLAG_ALL;
}

void transform::set_rotation_euler(const float3& euler) noexcept
{
    m_rotation = quaternion::rotation_euler(euler);
    m_dirty_flag = DIRTY_FLAG_ALL;
}

const float4& transform::get_rotation() const noexcept
{
    return m_rotation;
}

void transform::set_scale(float x, float y, float z) noexcept
{
    m_scale[0] = x;
    m_scale[1] = y;
    m_scale[2] = z;
    m_dirty_flag = DIRTY_FLAG_ALL;
}

void transform::set_scale(const float3& value) noexcept
{
    m_scale = value;
    m_dirty_flag = DIRTY_FLAG_ALL;
}

void transform::set_scale(float4_simd value) noexcept
{
    simd::store(value, m_scale);
    m_dirty_flag = DIRTY_FLAG_ALL;
}

const float3& transform::get_scale() const noexcept
{
    return m_scale;
}

const float4x4& transform::get_local_matrix() const noexcept
{
    return m_local_matrix;
}

const float4x4& transform::get_world_matrix() const noexcept
{
    return m_world_matrix;
}

void transform::update_local(const float4x4& local_matrix)
{
    update_local(simd::load(local_matrix));
}

void transform::update_local(const float4x4_simd& local_matrix)
{
    float4_simd scale, rotation, position;
    matrix_simd::decompose(local_matrix, scale, rotation, position);

    simd::store(scale, m_scale);
    simd::store(rotation, m_rotation);
    simd::store(position, m_position);

    simd::store(local_matrix, m_local_matrix);

    m_dirty_flag ^= DIRTY_FLAG_LOCAL;
    ++m_update_count;
}

void transform::update_world(const float4x4& world_matrix)
{
    m_world_matrix = world_matrix;
    m_dirty_flag ^= DIRTY_FLAG_WORLD;
    ++m_update_count;
}

void transform::update_world(const float4x4_simd& world_matrix)
{
    simd::store(world_matrix, m_world_matrix);
    m_dirty_flag ^= DIRTY_FLAG_WORLD;
    ++m_update_count;
}

std::size_t transform::get_update_count() const noexcept
{
    return m_update_count;
}
} // namespace violet