#pragma once

#include "graphics/material.hpp"
#include "graphics/renderer.hpp"

namespace violet::sample
{
class mmd_material : public material
{
public:
    mmd_material();

    void set_diffuse(const float4& diffuse);
    void set_specular(float3 specular, float specular_strength);
    void set_ambient(const float3& ambient);
    void set_toon_mode(std::uint32_t toon_mode);
    void set_spa_mode(std::uint32_t spa_mode);

    void set_tex(rhi_texture* texture, rhi_sampler* sampler);
    void set_toon(rhi_texture* texture, rhi_sampler* sampler);
    void set_spa(rhi_texture* texture, rhi_sampler* sampler);

    void set_edge(const float4& edge_color, float edge_size);
};

class mmd_renderer : public renderer
{
public:
    virtual void render(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera) override;

private:
    void add_skinning_pass(render_graph& graph);
};
} // namespace violet::sample