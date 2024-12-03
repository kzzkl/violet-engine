#include "graphics/materials/pbr_material.hpp"

namespace violet
{
struct pbr_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/pbr_material.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"tangent", RHI_FORMAT_R32G32B32_FLOAT},
        {"uv", RHI_FORMAT_R32G32_FLOAT},
    };
};

struct pbr_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/pbr_material.hlsl";
};

pbr_material::pbr_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    rdg_render_pipeline pipeline = {
        .vertex_shader = render_device::instance().get_shader<pbr_material_vs>(),
        .fragment_shader = render_device::instance().get_shader<pbr_material_fs>(),
    };
    pipeline.depth_stencil.depth_enable = true;
    pipeline.depth_stencil.depth_write_enable = true;
    pipeline.depth_stencil.depth_compare_op = RHI_COMPARE_OP_GREATER;
    pipeline.depth_stencil.stencil_enable = true;
    pipeline.depth_stencil.stencil_front = {
        .compare_op = RHI_COMPARE_OP_ALWAYS,
        .pass_op = RHI_STENCIL_OP_REPLACE,
        .depth_fail_op = RHI_STENCIL_OP_KEEP,
        .reference = LIGHTING_PHYSICAL,
    };
    pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;
    set_pipeline(pipeline);

    set_albedo({1.0f, 1.0f, 1.0f});
}

void pbr_material::set_albedo(const vec3f& albedo)
{
    get_constant().albedo = albedo;
}

void pbr_material::set_albedo(rhi_texture* albedo) {}

void pbr_material::set_roughness(float roughness)
{
    get_constant().roughness = roughness;
}

void pbr_material::set_roughness(rhi_texture* roughness) {}

void pbr_material::set_metallic(float metallic)
{
    get_constant().metallic = metallic;
}

void pbr_material::set_metallic(rhi_texture* metallic) {}

void pbr_material::set_normal(rhi_texture* normal) {}
} // namespace violet