#pragma once

#include "entity.hpp"
#include "math.hpp"
#include "mmd_bezier.hpp"
#include "pipeline_parameter.hpp"
#include <string>
#include <vector>

namespace ash::sample::mmd
{
struct mmd_node
{
    std::string name;
    std::uint32_t index;
    std::int32_t layer;

    bool deform_after_physics;

    bool inherit_local_flag;
    bool is_inherit_rotation;
    bool is_inherit_translation;

    ecs::entity inherit_node{ecs::INVALID_ENTITY};
    float inherit_weight;
    math::float3 inherit_translate{0.0f, 0.0f, 0.0f};
    math::float4 inherit_rotate{0.0f, 0.0f, 0.0f, 1.0f};

    math::float3 initial_position{0.0f, 0.0f, 0.0f};
    math::float4 initial_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    math::float3 initial_scale{1.0f, 1.0f, 1.0f};
    math::float4x4 initial_inverse;
};

struct mmd_node_animation
{
    struct key
    {
        std::int32_t frame;
        math::float3 translate;
        math::float4 rotate;

        mmd_bezier tx_bezier;
        mmd_bezier ty_bezier;
        mmd_bezier tz_bezier;
        mmd_bezier r_bezier;
    };

    std::size_t offset;
    std::vector<key> keys;

    math::float3 animation_translate{0.0f, 0.0f, 0.0f};
    math::float4 animation_rotate{0.0f, 0.0f, 0.0f, 1.0f};
    math::float3 base_animation_translate{0.0f, 0.0f, 0.0f};
    math::float4 base_animation_rotate{0.0f, 0.0f, 0.0f, 1.0f};
};

struct mmd_ik_solver
{
    struct key
    {
        std::int32_t frame;
        bool enable;
    };

    bool enable;
    float limit_angle;

    std::size_t offset;
    std::vector<key> keys;
    bool base_animation;

    ecs::entity ik_target;
    std::vector<ecs::entity> links;

    std::uint32_t loop_count;
};

struct mmd_ik_link
{
    math::float4 ik_rotate{0.0f, 0.0f, 0.0f, 1.0f};

    bool enable_limit;
    math::float3 limit_max;
    math::float3 limit_min;
    math::float3 prev_angle;
    math::float4 save_ik_rotate;
    float plane_mode_angle;
};

struct mmd_skeleton
{
    std::vector<ecs::entity> nodes;
    std::vector<ecs::entity> sorted_nodes;

    std::vector<math::float4x4> local;
    std::vector<math::float4x4> world;
};
} // namespace ash::sample::mmd