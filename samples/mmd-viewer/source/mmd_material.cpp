#include "mmd_material.hpp"

namespace violet
{
struct mmd_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/mmd_material.hlsl";
};

struct mmd_material_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/mmd_material.hlsl";
};

struct mmd_outline_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/mmd_outline.hlsl";
};

struct mmd_outline_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/mmd_outline.hlsl";
};

mmd_material::mmd_material()
    : mesh_material(MATERIAL_TOON)
{
    auto& device = render_device::instance();

    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = device.get_shader<mmd_material_vs>();
    pipeline.fragment_shader = device.get_shader<mmd_material_fs>();
    pipeline.rasterizer_state = device.get_rasterizer_state(RHI_CULL_MODE_BACK);
    pipeline.depth_stencil_state = device.get_depth_stencil_state<
        true,
        true,
        RHI_COMPARE_OP_GREATER,
        true,
        material_stencil_state<SHADING_MODEL_TOON>::value,
        material_stencil_state<SHADING_MODEL_TOON>::value>();
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
    auto& device = render_device::instance();

    auto& pipeline = get_pipeline();
    pipeline.vertex_shader = device.get_shader<mmd_outline_vs>();
    pipeline.fragment_shader = device.get_shader<mmd_outline_fs>();
    pipeline.rasterizer_state = device.get_rasterizer_state(RHI_CULL_MODE_FRONT);
    pipeline.depth_stencil_state = device.get_depth_stencil_state<
        true,
        true,
        RHI_COMPARE_OP_GREATER,
        true,
        material_stencil_state<SHADING_MODEL_UNLIT>::value,
        material_stencil_state<SHADING_MODEL_UNLIT>::value>();
    pipeline.primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

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
    constant.color.x = color.x;
    constant.color.y = color.y;
    constant.color.z = color.z;
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