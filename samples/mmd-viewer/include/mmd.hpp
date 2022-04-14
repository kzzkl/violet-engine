#pragma once

#include "entity.hpp"
#include "math.hpp"
#include "render_parameter.hpp"
#include <string>
#include <vector>

namespace ash::sample::mmd
{
struct mmd_bone
{
    std::string name;
    std::uint32_t index;
    std::int32_t layer;

    bool deform_after_physics;

    bool inherit_local_flag;
    bool inherit_rotation_flag;
    bool inherit_translation_flag;

    ecs::entity inherit_node;
    float inherit_weight;

    math::float3 inherit_translate;
    math::float4 inherit_rotate;

    math::float3 initial_position;
    math::float4 initial_rotation;
    math::float3 initial_scale;
    math::float4x4 initial_inverse;

    math::float3 animation_translate;
    math::float4 animation_rotate;
    math::float3 base_animation_translate;
    math::float4 base_animation_rotate;

    /*math::float3 translate;
    math::float4 rotate;
    math::float3 scale;

    math::float4x4 parent_matrix;
    math::float4x4 world_matrix;*/
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