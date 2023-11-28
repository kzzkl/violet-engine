#pragma once

#include "bezier.hpp"
#include <vector>

namespace violet::sample
{
class mmd_animator
{
public:
    struct animation_key
    {
        std::int32_t frame;
        float3 translate;
        float4 rotate;

        bezier tx_bezier;
        bezier ty_bezier;
        bezier tz_bezier;
        bezier r_bezier;
    };

    struct ik_key
    {
        std::int32_t frame;
        bool enable;
    };

    struct motion
    {
        std::size_t offset;
        std::vector<animation_key> animation_keys;
        std::vector<ik_key> ik_keys;

        float3 translation{0.0f, 0.0f, 0.0f};
        float4 rotation{0.0f, 0.0f, 0.0f, 1.0f};
        float3 base_translation{0.0f, 0.0f, 0.0f};
        float4 base_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct morph_key
    {
        std::int32_t frame;
        float weight;
    };

    struct morph
    {
        std::size_t offset;
        std::vector<morph_key> morph_keys;
    };

    std::vector<motion> motions;
    std::vector<morph> morphs;
};
} // namespace violet::sample