#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet::sample
{
struct mmd_material
{
    float4 diffuse;
    float3 specular;
    float specular_strength;
    float4 edge_color;
    float3 ambient;
    float edge_size;
    std::uint32_t toon_mode;
    std::uint32_t spa_mode;
};

struct mmd_skinning_bone
{
    float4x3 offset;
    float4 quaternion;
};

class mmd_render_graph : public render_graph
{
public:
    mmd_render_graph(renderer* renderer);

private:
};
} // namespace violet::sample