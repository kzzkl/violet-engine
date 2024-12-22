#pragma once

#include "ecs/entity.hpp"
#include "math/types.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet::sample
{
struct mmd_ik_solver
{
    bool enable{true};
    float limit;
    std::size_t offset;
    bool base_animation;
    std::int32_t target;
    std::vector<std::uint32_t> links;
    std::uint32_t iteration_count;
};

struct mmd_ik_link
{
    vec4f rotate{0.0f, 0.0f, 0.0f, 1.0f};
    bool enable_limit;
    vec3f limit_max;
    vec3f limit_min;
    vec3f prev_angle;
    float plane_mode_angle;
};

struct mmd_bone
{
    std::string name;
    entity entity;
    std::uint32_t index;

    bool update_after_physics;

    bool inherit_local_flag;
    bool is_inherit_rotation;
    bool is_inherit_translation;

    vec3f position;
    vec4f rotation;

    std::size_t inherit_index;
    float inherit_weight;
    vec3f inherit_translation{0.0f, 0.0f, 0.0f};
    vec4f inherit_rotation{0.0f, 0.0f, 0.0f, 1.0f};

    vec3f initial_position;

    std::unique_ptr<mmd_ik_solver> ik_solver;
    std::unique_ptr<mmd_ik_link> ik_link;
};

struct mmd_skeleton_component
{
    std::vector<mmd_bone> bones;
    std::vector<std::size_t> sorted_bones;
};
} // namespace violet::sample