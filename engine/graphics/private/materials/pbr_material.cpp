#include "graphics/materials/pbr_material.hpp"
#include "graphics/shading_models/pbr_shading_model.hpp"

namespace violet
{
struct pbr_material_cs : public material_resolve_cs
{
    static constexpr std::string_view path = "assets/shaders/materials/pbr_material.hlsl";
};

pbr_material::pbr_material()
{
    auto& device = render_device::instance();

    set_pipeline<pbr_shading_model>(
        {
            .vertex_shader = device.get_shader<visibility_vs>(),
            .fragment_shader = device.get_shader<visibility_fs>(),
            .rasterizer_state = device.get_rasterizer_state<RHI_CULL_MODE_NONE>(),
            .depth_stencil_state =
                device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
        },
        {
            .compute_shader = device.get_shader<pbr_material_cs>(),
        });
    set_surface_type(SURFACE_TYPE_OPAQUE);

    set_albedo({1.0f, 1.0f, 1.0f});
}

void pbr_material::set_albedo(const vec3f& albedo)
{
    get_constant().albedo = albedo;
}

void pbr_material::set_albedo(texture_2d* albedo)
{
    std::uint32_t albedo_texture = albedo->get_srv()->get_bindless();
    get_constant().albedo_texture = albedo_texture;
    set_opacity_mask(albedo_texture);
}

vec3f pbr_material::get_albedo() const
{
    return get_constant().albedo;
}

void pbr_material::set_roughness(float roughness)
{
    get_constant().roughness = roughness;
}

float pbr_material::get_roughness() const
{
    return get_constant().roughness;
}

void pbr_material::set_metallic(float metallic)
{
    get_constant().metallic = metallic;
}

float pbr_material::get_metallic() const
{
    return get_constant().metallic;
}

void pbr_material::set_roughness_metallic(texture_2d* roughness_metallic)
{
    get_constant().roughness_metallic_texture = roughness_metallic->get_srv()->get_bindless();
}

void pbr_material::set_emissive(const vec3f& emissive)
{
    get_constant().emissive = emissive;
}

void pbr_material::set_emissive(texture_2d* emissive)
{
    get_constant().emissive_texture = emissive->get_srv()->get_bindless();
}

vec3f pbr_material::get_emissive() const
{
    return get_constant().emissive;
}

void pbr_material::set_normal(texture_2d* normal)
{
    get_constant().normal_texture = normal->get_srv()->get_bindless();
}
} // namespace violet