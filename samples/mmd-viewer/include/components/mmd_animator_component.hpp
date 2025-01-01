#pragma once

#include "bezier.hpp"
#include <vector>

namespace violet
{
struct mmd_animation_key
{
    std::int32_t frame;
    vec3f translate;
    vec4f rotate;

    bezier tx_bezier;
    bezier ty_bezier;
    bezier tz_bezier;
    bezier r_bezier;
};

struct mmd_ik_key
{
    std::int32_t frame;
    bool enable;
};

struct mmd_motion
{
    std::size_t offset;
    std::vector<mmd_animation_key> animation_keys;
    std::vector<mmd_ik_key> ik_keys;

    vec3f translation{0.0f, 0.0f, 0.0f};
    vec4f rotation{0.0f, 0.0f, 0.0f, 1.0f};
    vec3f base_translation{0.0f, 0.0f, 0.0f};
    vec4f base_rotation{0.0f, 0.0f, 0.0f, 1.0f};
};

struct mmd_morph_key
{
    std::int32_t frame;
    float weight;
};

struct mmd_morph
{
    std::size_t offset;
    std::vector<mmd_morph_key> morph_keys;
};

struct mmd_animator_component
{
    std::vector<mmd_motion> motions;
    std::vector<mmd_morph> morphs;
};
} // namespace violet