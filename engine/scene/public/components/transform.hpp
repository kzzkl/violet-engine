#pragma once

#include "math/math.hpp"

namespace violet
{
class transform
{
public:
    enum dirty_flag : std::uint8_t
    {
        DIRTY_FLAG_NONE = 0,
        DIRTY_FLAG_LOCAL = 1,
        DIRTY_FLAG_WORLD = 1 << 1,
        DIRTY_FLAG_ALL = DIRTY_FLAG_LOCAL | DIRTY_FLAG_WORLD
    };

public:
    transform() noexcept;

    void set_position(float x, float y, float z) noexcept;
    void set_position(const float3& position) noexcept;
    void set_position(float4_simd position) noexcept;
    const float3& get_position() const noexcept;

    void set_rotation(const float4& quaternion) noexcept;
    void set_rotation(float4_simd quaternion) noexcept;
    void set_rotation_euler(const float3& euler) noexcept;
    const float4& get_rotation() const noexcept;

    void set_scale(float x, float y, float z) noexcept;
    void set_scale(const float3& value) noexcept;
    void set_scale(float4_simd value) noexcept;
    const float3& get_scale() const noexcept;

    const float4x4& get_local_matrix() const noexcept;
    const float4x4& get_world_matrix() const noexcept;

    void update_local(const float4x4& local_matrix);
    void update_local(const float4x4_simd& local_matrix);
    void update_world(const float4x4& world_matrix);
    void update_world(const float4x4_simd& world_matrix);

    std::size_t get_update_count() const noexcept;
    void reset_update_count() noexcept { m_update_count = 0; }

    std::uint8_t get_dirty_flag() const noexcept { return m_dirty_flag; }

private:
    float3 m_position;
    float4 m_rotation;
    float3 m_scale;

    float4x4 m_local_matrix;
    float4x4 m_world_matrix;

    std::uint8_t m_dirty_flag;
    std::size_t m_update_count;
};
} // namespace violet