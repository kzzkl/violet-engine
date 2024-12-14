#pragma once

#include "ecs/component.hpp"
#include "math/quaternion.hpp"

namespace violet
{
struct transform_component
{
    vec3f position{0.0f, 0.0f, 0.0f};
    vec4f rotation{0.0f, 0.0f, 0.0f, 1.0f};
    vec3f scale{1.0f, 1.0f, 1.0f};

    void lookat(const vec3f& target, const vec3f& up)
    {
        vec4f_simd t = math::load(target);
        vec4f_simd p = math::load(position);
        vec4f_simd u = math::load(up);

        vec4f_simd z_axis = vector::normalize(vector::sub(t, p));
        vec4f_simd x_axis = vector::normalize(vector::cross(u, z_axis));
        vec4f_simd y_axis = vector::cross(z_axis, x_axis);

        mat4f_simd rotation_matrix = {x_axis, y_axis, z_axis, vector::set(0.0f, 0.0f, 0.0f, 1.0f)};
        math::store(quaternion::from_matrix(rotation_matrix), rotation);
    }
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
};

template <>
struct component_trait<transform_world_component>
{
    using main_component = transform_component;
};
} // namespace violet