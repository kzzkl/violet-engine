#include "mmd_material.hpp"

namespace violet::sample
{
struct mmd_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "mmd-viewer/shaders/mmd_material.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"texcoord", RHI_FORMAT_R32G32_FLOAT},
    };
};

struct mmd_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "mmd-viewer/shaders/mmd_material.hlsl";
};

mmd_material::mmd_material()
    : mesh_material(MATERIAL_OPAQUE)
{
    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = render_device::instance().get_shader<mmd_material_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<mmd_material_fs>();
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
}

void mmd_material::set_diffuse(const vec4f& diffuse)
{
    get_constant().diffuse = diffuse;
}

void mmd_material::set_specular(vec3f specular, float specular_strength)
{
    auto& constant = get_constant();
    constant.specular = specular;
    constant.specular_strength = specular_strength;
}

void mmd_material::set_ambient(const vec3f& ambient)
{
    get_constant().ambient = ambient;
}

void mmd_material::set_diffuse(rhi_texture* texture)
{
    get_constant().diffuse_texture = texture->get_handle();
}

void mmd_material::set_toon(rhi_texture* texture)
{
    get_constant().toon_texture = texture->get_handle();
}

void mmd_material::set_environment(rhi_texture* texture)
{
    get_constant().environment_texture = texture->get_handle();
}

void mmd_material::set_environment_blend(std::uint32_t mode)
{
    get_constant().environment_blend_mode = mode;
}

void mmd_material::set_edge(const vec4f& edge_color, float edge_size)
{
    auto& constant = get_constant();
    constant.edge_color = edge_color;
    constant.edge_size = edge_size;
}
} // namespace violet::sample