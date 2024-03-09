#pragma once

#include "graphics/render_graph/render_graph.hpp"
#include <array>

namespace violet::sample
{
class preprocess_graph : public render_graph
{
public:
    preprocess_graph(renderer* renderer);

    void set_target(rhi_image* environment_map, rhi_image* irradiance_map);

private:
    std::array<rhi_ptr<rhi_image>, 6> m_irradiance_targets;
};

class pbr_material : public material
{
public:
    void set_albedo(const float3& albedo);
    void set_metalness(float metalness);
    void set_roughness(float roughness);

    virtual std::vector<std::string> get_layout() const override;
};

class pbr_render_graph : public render_graph
{
public:
    pbr_render_graph(renderer* renderer);
};
} // namespace violet::sample