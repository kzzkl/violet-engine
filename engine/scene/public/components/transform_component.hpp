#pragma once

#include "ecs/component.hpp"
#include "math/matrix.hpp"
#include "math/quaternion.hpp"

namespace violet
{
class transform_component
{
public:
    void set_position(const vec3f& position) noexcept
    {
        m_position = position;
        m_dirty = TRANSFORM_DIRTY_ALL;
    }

    void set_position(vec4f_simd position) noexcept
    {
        math::store(position, m_position);
        m_dirty = TRANSFORM_DIRTY_ALL;
    }

    const vec3f& get_position() const noexcept
    {
        return m_position;
    }

    void set_rotation(const vec4f& rotation) noexcept
    {
        m_rotation = rotation;
        m_dirty = TRANSFORM_DIRTY_ALL;
    }

    void set_rotation(vec4f_simd rotation) noexcept
    {
        math::store(rotation, m_rotation);
        m_dirty = TRANSFORM_DIRTY_ALL;
    }

    const vec4f& get_rotation() const noexcept
    {
        return m_rotation;
    }

    void set_scale(const vec3f& scale) noexcept
    {
        m_scale = scale;
        m_dirty = TRANSFORM_DIRTY_ALL;
    }

    void set_scale(vec4f_simd scale) noexcept
    {
        math::store(scale, m_scale);
        m_dirty = TRANSFORM_DIRTY_ALL;
    }

    const vec3f& get_scale() const noexcept
    {
        return m_scale;
    }

    void lookat(const vec3f& target, const vec3f& up = {0.0f, 1.0f, 0.0f})
    {
        vec4f_simd t = math::load(target);
        vec4f_simd p = math::load(m_position);
        vec4f_simd u = math::load(up);

        vec4f_simd z_axis = vector::normalize(vector::sub(t, p));
        vec4f_simd x_axis = vector::normalize(vector::cross(u, z_axis));
        vec4f_simd y_axis = vector::cross(z_axis, x_axis);

        mat4f_simd rotation_matrix = {x_axis, y_axis, z_axis, vector::set(0.0f, 0.0f, 0.0f, 1.0f)};
        math::store(quaternion::from_matrix(rotation_matrix), m_rotation);
    }

private:
    friend class transform_system;

    enum transform_dirty_flag
    {
        TRANSFORM_DIRTY_LOCAL = 1 << 0,
        TRANSFORM_DIRTY_WORLD = 1 << 1,
        TRANSFORM_DIRTY_ALL = TRANSFORM_DIRTY_LOCAL | TRANSFORM_DIRTY_WORLD,
    };
    using transform_dirty_flags = std::uint32_t;

    bool is_world_dirty() const noexcept
    {
        return m_dirty & TRANSFORM_DIRTY_WORLD;
    }

    void set_world_dirty() const noexcept
    {
        m_dirty |= TRANSFORM_DIRTY_WORLD;
    }

    void clear_world_dirty() const noexcept
    {
        m_dirty &= ~TRANSFORM_DIRTY_WORLD;
    }

    bool is_local_dirty() const noexcept
    {
        return m_dirty & TRANSFORM_DIRTY_LOCAL;
    }

    void set_local_dirty() const noexcept
    {
        m_dirty |= TRANSFORM_DIRTY_LOCAL;
    }

    void clear_local_dirty() const noexcept
    {
        m_dirty &= ~TRANSFORM_DIRTY_LOCAL;
    }

    vec3f m_position{0.0f, 0.0f, 0.0f};
    vec4f m_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    vec3f m_scale{1.0f, 1.0f, 1.0f};

    mutable transform_dirty_flags m_dirty{0};
};

struct transform_local_component
{
    mat4f matrix;
};

template <>
struct component_trait<transform_local_component>
{
    using main_component = transform_component;
};

struct transform_world_component
{
    mat4f matrix;

    vec3f get_position() const
    {
        return vec3f{matrix[3][0], matrix[3][1], matrix[3][2]};
    }

    vec3f get_forward() const
    {
        vec4f_simd forward = vector::set(0.0f, 0.0f, 1.0f, 0.0f);
        mat4f_simd m = math::load(matrix);
        forward = matrix::mul(forward, m);

        vec3f result;
        math::store(forward, result);
        return result;
    }
};

template <>
struct component_trait<transform_world_component>
{
    using main_component = transform_component;
};
} // namespace violet