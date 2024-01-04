#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet::sample
{
class pre_process_graph : public render_graph
{
public:
    pre_process_graph(renderer* renderer);

    void set_parameter(
        rhi_image* ambient_map,
        rhi_sampler* ambient_sampler,
        rhi_image* irradiance_map);

private:
    rhi_ptr<rhi_parameter> m_parameter;

    rhi_ptr<rhi_image> m_render_target;
    rhi_ptr<rhi_framebuffer> m_framebuffer;

    renderer* m_renderer;
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