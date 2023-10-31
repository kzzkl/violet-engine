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

class mmd_render_graph : public render_graph
{
public:
    mmd_render_graph(graphics_context* context);

private:
};
} // namespace violet::sample