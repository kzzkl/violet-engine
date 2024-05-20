#pragma once

#include "graphics/material.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "math/math.hpp"
#include <array>

namespace violet::sample
{
class preprocess_graph : public render_graph
{
public:
    preprocess_graph();

    void set_target(rhi_texture* irradiance_map);

private:
    std::array<rhi_ptr<rhi_texture>, 6> m_irradiance_targets;
};

class pbr_material : public material
{
public:
    // pbr_material(renderer* renderer, material_layout* layout);

    void set_albedo(const float3& albedo);
    void set_metalness(float metalness);
    void set_roughness(float roughness);
};

class pbr_render_graph : public render_graph
{
public:
    pbr_render_graph(renderer* renderer);
};
} // namespace violet::sample