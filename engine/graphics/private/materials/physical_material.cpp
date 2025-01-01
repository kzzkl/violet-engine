#include "graphics/materials/physical_material.hpp"

namespace violet
{
struct physical_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/materials/physical_material.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"tangent", RHI_FORMAT_R32G32B32_FLOAT},
        {"texcoord", RHI_FORMAT_R32G32_FLOAT},
    };
};

struct physical_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/materials/physical_material.hlsl";
};

physical_material::physical_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = render_device::instance().get_shader<physical_material_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<physical_material_fs>();
    pipeline.depth_stencil.depth_enable = true;
    pipeline.depth_stencil.depth_write_enable = true;
    pipeline.depth_stencil.depth_compare_op = RHI_COMPARE_OP_GREATER;
    pipeline.depth_stencil.stencil_enable = true;
    pipeline.depth_stencil.stencil_front = {
        .compare_op = RHI_COMPARE_OP_ALWAYS,
        .pass_op = RHI_STENCIL_OP_REPLACE,
        .depth_fail_op = RHI_STENCIL_OP_KEEP,
        .reference = SHADING_MODEL_PHYSICAL,
    };
    pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

    set_albedo({1.0f, 1.0f, 1.0f});
}

void physical_material::set_albedo(const vec3f& albedo)
{
    get_constant().albedo = albedo;
}

void physical_material::set_albedo(rhi_texture* albedo)
{
    get_constant().albedo_texture = albedo->get_handle();
}

void physical_material::set_roughness(float roughness)
{
    get_constant().roughness = roughness;
}

void physical_material::set_metallic(float metallic)
{
    get_constant().metallic = metallic;
}

void physical_material::set_roughness_metallic(rhi_texture* roughness_metallic)
{
    get_constant().roughness_metallic_texture = roughness_metallic->get_handle();
}

void physical_material::set_emissive(const vec3f& emissive)
{
    get_constant().emissive = emissive;
}

void physical_material::set_emissive(rhi_texture* emissive)
{
    get_constant().emissive_texture = emissive->get_handle();
}

void physical_material::set_normal(rhi_texture* normal)
{
    get_constant().normal_texture = normal->get_handle();
}
} // namespace violet