#pragma once

#include "ecs/entity.hpp"
#include "math/math.hpp"

namespace violet::scene
{
class transform
{
public:
    transform() noexcept;

    void sync(const math::float4x4& parent_matrix);
    void sync(const math::float4x4_simd& parent_matrix);
    void sync(const math::float4x4& parent_matrix, const math::float4x4& world_matrix);
    void sync(const math::float4x4_simd& parent_matrix, const math::float4x4_simd& world_matrix);

    void position(const math::float3& value) noexcept
    {
        m_position = value;
        m_dirty = true;
    }

    void position(math::float4_simd value) noexcept
    {
        math::simd::store(value, m_position);
        m_dirty = true;
    }

    void rotation(const math::float4& value) noexcept
    {
        m_rotation = value;
        m_dirty = true;
    }

    void rotation(math::float4_simd value) noexcept
    {
        math::simd::store(value, m_rotation);
        m_dirty = true;
    }

    void rotation_euler(const math::float3& euler)
    {
        m_rotation = math::quaternion::rotation_euler(euler);
        m_dirty = true;
    }

    void scale(const math::float3& value) noexcept
    {
        m_scale = value;
        m_dirty = true;
    }

    void scale(math::float4_simd value) noexcept
    {
        math::simd::store(value, m_scale);
        m_dirty = true;
    }

    void to_world(const math::float4x4& world_matrix) noexcept
    {
        m_to_world = world_matrix;
        m_dirty = true;
    }

    void to_world(const math::float4x4_simd& world_matrix) noexcept
    {
        math::simd::store(world_matrix, m_to_world);
        m_dirty = true;
    }

    void in_scene(bool in_scene) noexcept { m_in_scene = in_scene; }

    const math::float3& position() const noexcept { return m_position; }
    const math::float4& rotation() const noexcept { return m_rotation; }
    const math::float3& scale() const noexcept { return m_scale; }

    const math::float4x4& to_parent() const noexcept { return m_to_parent; }
    const math::float4x4& to_world() const noexcept { return m_to_world; }

    math::float3 forward() const noexcept
    {
        return math::float3{m_to_world[2][0], m_to_world[2][1], m_to_world[2][2]};
    }

    bool in_scene() const noexcept { return m_in_scene; }

    bool dirty() const noexcept { return m_dirty; }
    void mark_dirty() noexcept { m_dirty = true; }

    std::size_t sync_count() const noexcept { return m_sync_count; }
    void reset_sync_count() noexcept { m_sync_count = 0; }

private:
    math::float3 m_position;
    math::float4 m_rotation;
    math::float3 m_scale;

    math::float4x4 m_to_parent;
    math::float4x4 m_to_world;

    bool m_in_scene;

    bool m_dirty;
    std::size_t m_sync_count;
};
} // namespace violet::scene