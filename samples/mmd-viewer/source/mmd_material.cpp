#include "mmd_material.hpp"

namespace violet
{
struct mmd_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/mmd_material.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"texcoord", RHI_FORMAT_R32G32_FLOAT},
    };
};

struct mmd_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/mmd_material.hlsl";
};

struct mmd_outline_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/mmd_outline.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"tangent", RHI_FORMAT_R32G32B32_FLOAT},
        {"smooth_normal", RHI_FORMAT_R32G32B32_FLOAT},
        {"outline", RHI_FORMAT_R32_FLOAT},
    };
};

struct mmd_outline_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/mmd_outline.hlsl";
};

mmd_material::mmd_material()
    : mesh_material(MATERIAL_TOON)
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
        .reference = SHADING_MODEL_TOON,
    };
    pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;
    pipeline.rasterizer.cull_mode = RHI_CULL_MODE_BACK;
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

void mmd_material::set_diffuse(const texture_2d* texture)
{
    get_constant().diffuse_texture = texture->get_srv()->get_bindless();
}

void mmd_material::set_toon(const texture_2d* texture)
{
    get_constant().toon_texture = texture->get_srv()->get_bindless();
}

void mmd_material::set_environment(const texture_2d* texture)
{
    get_constant().environment_texture = texture->get_srv()->get_bindless();
}

void mmd_material::set_environment_blend(std::uint32_t mode)
{
    get_constant().environment_blend_mode = mode;
}

void mmd_material::set_ramp(const texture_2d* texture)
{
    get_constant().ramp_texture = texture->get_srv()->get_bindless();
}

mmd_outline_material::mmd_outline_material()
    : mesh_material(MATERIAL_TOON)
{
    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = render_device::instance().get_shader<mmd_outline_vs>();
    pipeline.fragment_shader = render_device::instance().get_shader<mmd_outline_fs>();
    pipeline.depth_stencil.depth_enable = true;
    pipeline.depth_stencil.depth_write_enable = true;
    pipeline.depth_stencil.depth_compare_op = RHI_COMPARE_OP_GREATER;
    pipeline.depth_stencil.stencil_enable = true;
    pipeline.depth_stencil.stencil_front = {
        .compare_op = RHI_COMPARE_OP_ALWAYS,
        .pass_op = RHI_STENCIL_OP_REPLACE,
        .depth_fail_op = RHI_STENCIL_OP_KEEP,
        .reference = SHADING_MODEL_UNLIT,
    };
    pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;
    pipeline.rasterizer.cull_mode = RHI_CULL_MODE_FRONT;

    get_constant() = {
        .color = {1.0f, 1.0f, 1.0f},
        .width = 1.0f,
        .z_offset = 0.0f,
        .strength = 1.0f,
    };
}

void mmd_outline_material::set_color(const vec4f& color)
{
    auto& constant = get_constant();
    constant.color.r = color.r;
    constant.color.g = color.g;
    constant.color.b = color.b;
}

void mmd_outline_material::set_width(float width)
{
    auto& constant = get_constant();
    constant.width = width;
}

void mmd_outline_material::set_z_offset(float z_offset)
{
    auto& constant = get_constant();
    constant.z_offset = z_offset;
}

void mmd_outline_material::set_strength(float strength)
{
    auto& constant = get_constant();
    constant.strength = strength;
}
} // namespace violet