#include "mmd_renderer.hpp"

namespace violet::sample
{

mmd_material::mmd_material()
{
}

void mmd_material::set_diffuse(const float4& diffuse)
{
}

void mmd_material::set_specular(float3 specular, float specular_strength)
{
}

void mmd_material::set_ambient(const float3& ambient)
{
}

void mmd_material::set_toon_mode(std::uint32_t toon_mode)
{
}

void mmd_material::set_spa_mode(std::uint32_t spa_mode)
{
}

void mmd_material::set_tex(rhi_texture* texture, rhi_sampler* sampler)
{
}

void mmd_material::set_toon(rhi_texture* texture, rhi_sampler* sampler)
{
}

void mmd_material::set_spa(rhi_texture* texture, rhi_sampler* sampler)
{
}

void mmd_material::set_edge(const float4& edge_color, float edge_size)
{
}

void mmd_renderer::render(
    render_graph& graph,
    const render_context& context,
    const render_camera& camera)
{
}

void mmd_renderer::add_skinning_pass(render_graph& graph)
{
}
} // namespace violet::sample