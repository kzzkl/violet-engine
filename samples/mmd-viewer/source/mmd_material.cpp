#include "mmd_material.hpp"
#include "graphics/shading_models/unlit_shading_model.hpp"

namespace violet
{
mmd_material::mmd_material()
    : material_instance("mmd_material", "assets/shaders/mmd_material.hlsl")
{
    set_cull_mode(RHI_CULL_MODE_BACK);
    set_surface_type(SURFACE_TYPE_OPAQUE);
    set_shading_model<toon_shading_model>();
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
    : material_instance("mmd_outline_material", "assets/shaders/mmd_outline.hlsl")
{
    set_cull_mode(RHI_CULL_MODE_FRONT);
    set_surface_type(SURFACE_TYPE_OPAQUE);
    set_shading_model<unlit_shading_model>();

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
    : shading_model("toon_shading_model", 0, "assets/shaders/mmd_toon.hlsl")
{
}
} // namespace violet