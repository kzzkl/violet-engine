#include "graphics/materials/physical_material.hpp"

namespace violet
{
struct physical_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/physical_material.hlsl";
};

struct physical_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/physical_material.hlsl";
};

physical_material::physical_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& device = render_device::instance();

    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = device.get_shader<physical_material_vs>();
    pipeline.fragment_shader = device.get_shader<physical_material_fs>();
    pipeline.depth_stencil_state = device.get_depth_stencil_state<
        true,
        true,
        RHI_COMPARE_OP_GREATER,
        true,
        material_stencil_state<SHADING_MODEL_PHYSICAL>::value,
        material_stencil_state<SHADING_MODEL_PHYSICAL>::value>();
    pipeline.rasterizer_state = device.get_rasterizer_state<RHI_CULL_MODE_BACK>();

    set_albedo({1.0f, 1.0f, 1.0f});
}

void physical_material::set_albedo(const vec3f& albedo)
{
    get_constant().albedo = albedo;
}

void physical_material::set_albedo(texture_2d* albedo)
{
    get_constant().albedo_texture = albedo->get_srv()->get_bindless();
}

vec3f physical_material::get_albedo() const
{
    return get_constant().albedo;
}

void physical_material::set_roughness(float roughness)
{
    get_constant().roughness = roughness;
}

float physical_material::get_roughness() const
{
    return get_constant().roughness;
}

void physical_material::set_metallic(float metallic)
{
    get_constant().metallic = metallic;
}

float physical_material::get_metallic() const
{
    return get_constant().metallic;
}

void physical_material::set_roughness_metallic(texture_2d* roughness_metallic)
{
    get_constant().roughness_metallic_texture = roughness_metallic->get_srv()->get_bindless();
}

void physical_material::set_emissive(const vec3f& emissive)
{
    get_constant().emissive = emissive;
}

void physical_material::set_emissive(texture_2d* emissive)
{
    get_constant().emissive_texture = emissive->get_srv()->get_bindless();
}

vec3f physical_material::get_emissive() const
{
    return get_constant().emissive;
}

void physical_material::set_normal(texture_2d* normal)
{
    get_constant().normal_texture = normal->get_srv()->get_bindless();
}
} // namespace violet