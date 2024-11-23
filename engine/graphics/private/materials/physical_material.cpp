#include "graphics/materials/physical_material.hpp"

namespace violet
{
struct physical_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/source/materials/physical.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"tangent", RHI_FORMAT_R32G32B32_FLOAT},
        {"uv", RHI_FORMAT_R32G32_FLOAT}};
};

struct physical_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/source/materials/physical.hlsl";
};

physical_material::physical_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    rdg_render_pipeline pipeline = {
        .vertex_shader = render_device::instance().get_shader<physical_material_vs>(),
        .fragment_shader = render_device::instance().get_shader<physical_material_fs>(),
    };
    set_pipeline(pipeline);

    set_albedo({1.0f, 1.0f, 1.0f});
}

void physical_material::set_albedo(const vec3f& albedo)
{
    get_constant().albedo = albedo;
}

void physical_material::set_roughness(float roughness)
{
    get_constant().roughness = roughness;
}

void physical_material::set_metallic(float metallic)
{
    get_constant().metallic = metallic;
}

void physical_material::set_albedo(rhi_texture* albedo, rhi_sampler* sampler) {}

void physical_material::set_roughness(rhi_texture* roughness, rhi_sampler* sampler) {}

void physical_material::set_metallic(rhi_texture* metallic, rhi_sampler* sampler) {}

void physical_material::set_normal(rhi_texture* normal, rhi_sampler* sampler) {}
} // namespace violet