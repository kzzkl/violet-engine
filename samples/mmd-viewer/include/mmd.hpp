#pragma once

#include "entity.hpp"
#include "math.hpp"
#include "render_parameter.hpp"
#include <string>
#include <vector>

namespace ash::sample::mmd
{
struct mmd_ik_link
{
    ecs::entity node;
    bool enable_limit;
    math::float3 limit_max;
    math::float3 limit_min;
    math::float3 prev_angle;
    math::float4 save_ik_rotate;
    float plane_mode_angle;
};

struct mmd_bone
{
    std::string name;
    std::uint32_t index;
    std::int32_t layer;

    bool deform_after_physics;

    bool inherit_local_flag;
    bool inherit_rotation_flag;
    bool inherit_translation_flag;

    ecs::entity inherit_node{ecs::INVALID_ENTITY};
    float inherit_weight;
    math::float3 inherit_translate;
    math::float4 inherit_rotate;

    bool enable_ik;
    bool enable_ik_solver{true};
    ecs::entity ik_target;
    std::uint32_t loop_count;
    float limit_angle;
    bool base_animation;
    math::float4 ik_rotate;
    std::vector<mmd_ik_link> links;

    math::float3 initial_position;
    math::float4 initial_rotation;
    math::float3 initial_scale;
    math::float4x4 initial_inverse;

    math::float3 animation_translate;
    math::float4 animation_rotate;
    math::float3 base_animation_translate;
    math::float4 base_animation_rotate;
};

struct mmd_skeleton
{
    std::vector<ecs::entity> nodes;
    std::vector<ecs::entity> sorted_nodes;
    std::vector<math::float4x4> transform;

    std::unique_ptr<graphics::render_parameter> parameter;
};

struct mmd_animation_key
{
    std::int32_t frame;
    math::float3 translate;
    math::float4 rotate;

    math::float4 tx_bezier;
    math::float4 ty_bezier;
    math::float4 tz_bezier;
    math::float4 r_bezier;
};

struct mmd_animation
{
    std::vector<mmd_animation_key> keys;
};
} // namespace ash::sample::mmd