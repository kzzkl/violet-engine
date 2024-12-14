#include "mmd_material.hpp"

namespace violet::sample
{
mmd_material::mmd_material()
    : material(MATERIAL_OPAQUE)
{
}

void mmd_material::set_diffuse(const vec4f& diffuse) {}

void mmd_material::set_specular(vec3f specular, float specular_strength) {}

void mmd_material::set_ambient(const vec3f& ambient) {}

void mmd_material::set_toon_mode(std::uint32_t toon_mode) {}

void mmd_material::set_spa_mode(std::uint32_t spa_mode) {}

void mmd_material::set_albedo(rhi_texture* texture) {}

void mmd_material::set_toon(rhi_texture* texture) {}

void mmd_material::set_spa(rhi_texture* texture) {}

void mmd_material::set_edge(const vec4f& edge_color, float edge_size) {}
} // namespace violet::sample