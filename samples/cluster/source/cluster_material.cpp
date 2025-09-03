#include "cluster_material.hpp"
#include "graphics/shading_models/pbr_shading_model.hpp"

namespace violet
{
struct cluster_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/cluster_material.hlsl";
};

struct cluster_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/cluster_material.hlsl";
};

cluster_material::cluster_material()
{
    auto& device = render_device::instance();

    set_pipeline<pbr_shading_model>({
        .vertex_shader = device.get_shader<cluster_material_vs>(),
        .fragment_shader = device.get_shader<cluster_material_fs>(),
        .rasterizer_state = device.get_rasterizer_state<RHI_CULL_MODE_BACK>(),
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
    });
    set_surface_type(SURFACE_TYPE_OPAQUE);

    set_albedo({1.0f, 1.0f, 1.0f});
}

void cluster_material::set_albedo(const vec3f& albedo)
{
    get_constant().albedo = albedo;
}

void cluster_material::set_albedo(texture_2d* albedo)
{
    get_constant().albedo_texture = albedo->get_srv()->get_bindless();
}

vec3f cluster_material::get_albedo() const
{
    return get_constant().albedo;
}

void cluster_material::set_roughness(float roughness)
{
    get_constant().roughness = roughness;
}

float cluster_material::get_roughness() const
{
    return get_constant().roughness;
}

void cluster_material::set_metallic(float metallic)
{
    get_constant().metallic = metallic;
}

float cluster_material::get_metallic() const
{
    return get_constant().metallic;
}

void cluster_material::set_roughness_metallic(texture_2d* roughness_metallic)
{
    get_constant().roughness_metallic_texture = roughness_metallic->get_srv()->get_bindless();
}

void cluster_material::set_emissive(const vec3f& emissive)
{
    get_constant().emissive = emissive;
}

void cluster_material::set_emissive(texture_2d* emissive)
{
    get_constant().emissive_texture = emissive->get_srv()->get_bindless();
}

vec3f cluster_material::get_emissive() const
{
    return get_constant().emissive;
}

void cluster_material::set_normal(texture_2d* normal)
{
    get_constant().normal_texture = normal->get_srv()->get_bindless();
}
} // namespace violet