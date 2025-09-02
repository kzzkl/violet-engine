#include "mmd_material.hpp"
#include "graphics/shading_models/unlit_shading_model.hpp"

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

struct toon_cs : public shading_model_cs
{
    static constexpr std::string_view path = "assets/shaders/mmd_toon.hlsl";
};

mmd_material::mmd_material()
{
    auto& device = render_device::instance();

    set_pipeline<toon_shading_model>({
        .vertex_shader = device.get_shader<mmd_material_vs>(),
        .fragment_shader = device.get_shader<mmd_material_fs>(),
        .rasterizer_state = device.get_rasterizer_state(RHI_CULL_MODE_BACK),
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
    });
    set_surface_type(SURFACE_TYPE_OPAQUE);
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
{
    auto& device = render_device::instance();

    set_pipeline<unlit_shading_model>({
        .vertex_shader = device.get_shader<mmd_outline_vs>(),
        .fragment_shader = device.get_shader<mmd_outline_fs>(),
        .rasterizer_state = device.get_rasterizer_state(RHI_CULL_MODE_FRONT),
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
    });
    set_surface_type(SURFACE_TYPE_OPAQUE);

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

toon_shading_model::toon_shading_model()
    : shading_model(
          "Toon",
          {SHADING_GBUFFER_ALBEDO, SHADING_GBUFFER_NORMAL},
          {SHADING_AUXILIARY_BUFFER_AO})
{
}

const rdg_compute_pipeline& toon_shading_model::get_pipeline()
{
    auto& device = render_device::instance();

    bool has_ao = has_auxiliary_buffer(SHADING_AUXILIARY_BUFFER_AO);

    if (has_ao && m_pipeline_without_ao.compute_shader == nullptr)
    {
        std::vector<std::wstring> defines = {L"-DUSE_AO_BUFFER"};
        m_pipeline_with_ao.compute_shader = device.get_shader<toon_cs>(defines);
    }
    else if (!has_ao && m_pipeline_without_ao.compute_shader == nullptr)
    {
        m_pipeline_without_ao.compute_shader = device.get_shader<toon_cs>();
    }

    return has_ao ? m_pipeline_with_ao : m_pipeline_without_ao;
}
} // namespace violet