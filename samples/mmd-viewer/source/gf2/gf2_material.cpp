#include "gf2/gf2_material.hpp"
#include "graphics/resources/brdf_lut.hpp"
#include "mmd_material.hpp"

namespace violet
{
struct gf2_material_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_vs.hlsl";
};

struct gf2_material_base_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_base.hlsl";
};

struct gf2_material_face_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_face.hlsl";
};

struct gf2_material_eye_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_eye.hlsl";
};

struct gf2_material_eye_blend_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_eye_blend.hlsl";
};

struct gf2_material_hair_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_hair.hlsl";
};

struct gf2_material_plush_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/gf2/gf2_material_plush.hlsl";
};

gf2_material_base::gf2_material_base()
{
    auto& device = render_device::instance();

    set_cull_mode(RHI_CULL_MODE_BACK);
    set_shading_model<toon_shading_model>();
    set_surface_type(SURFACE_TYPE_OPAQUE);

    get_constant().brdf_lut = device.get_texture<brdf_lut>()->get_srv()->get_bindless();
}

void gf2_material_base::set_diffuse(const texture_2d* texture)
{
    get_constant().diffuse_texture = texture->get_srv()->get_bindless();
}

void gf2_material_base::set_normal(const texture_2d* texture)
{
    get_constant().normal_texture = texture->get_srv()->get_bindless();
}

void gf2_material_base::set_rmo(const texture_2d* texture)
{
    get_constant().rmo_texture = texture->get_srv()->get_bindless();
}

void gf2_material_base::set_ramp(const texture_2d* texture)
{
    get_constant().ramp_texture = texture->get_srv()->get_bindless();
}

rhi_shader* gf2_material_base::get_vertex_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_vs>(defines);
}

rhi_shader* gf2_material_base::get_fragment_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_base_fs>(defines);
}

gf2_material_face::gf2_material_face()
{
    auto& device = render_device::instance();

    set_cull_mode(RHI_CULL_MODE_BACK);
    set_shading_model<toon_shading_model>();
    set_surface_type(SURFACE_TYPE_OPAQUE);

    get_constant().brdf_lut = device.get_texture<brdf_lut>()->get_srv()->get_bindless();
}

void gf2_material_face::set_diffuse(const texture_2d* texture)
{
    get_constant().diffuse_texture = texture->get_srv()->get_bindless();
}

void gf2_material_face::set_sdf(const texture_2d* texture)
{
    get_constant().sdf_texture = texture->get_srv()->get_bindless();
}

void gf2_material_face::set_ramp(const texture_2d* texture)
{
    get_constant().ramp_texture = texture->get_srv()->get_bindless();
}

void gf2_material_face::set_face_dir(const vec3f& face_front_dir, const vec3f& face_left_dir)
{
    auto& constant = get_constant();
    constant.face_front_dir = face_front_dir;
    constant.face_left_dir = face_left_dir;
}

rhi_shader* gf2_material_face::get_vertex_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_vs>(defines);
}

rhi_shader* gf2_material_face::get_fragment_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_face_fs>(defines);
}

gf2_material_eye::gf2_material_eye()
{
    auto& device = render_device::instance();

    set_cull_mode(RHI_CULL_MODE_BACK);
    set_shading_model<toon_shading_model>();
    set_surface_type(SURFACE_TYPE_OPAQUE);
}

void gf2_material_eye::set_diffuse(const texture_2d* texture)
{
    get_constant().diffuse_texture = texture->get_srv()->get_bindless();
}

rhi_shader* gf2_material_eye::get_vertex_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_vs>(defines);
}

rhi_shader* gf2_material_eye::get_fragment_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_eye_fs>(defines);
}

gf2_material_eye_blend::gf2_material_eye_blend(bool is_add)
{
    auto& device = render_device::instance();

    set_cull_mode(RHI_CULL_MODE_BACK);

    const rhi_blend_state* default_blend_state = nullptr;
    if (is_add)
    {
        default_blend_state = device.get_blend_state<blend_state::attachment<
            true,
            RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
            RHI_BLEND_FACTOR_ONE,
            RHI_BLEND_OP_ADD,
            RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
            RHI_BLEND_FACTOR_ONE,
            RHI_BLEND_OP_ADD>>();
    }
    else
    {
        default_blend_state = device.get_blend_state<blend_state::attachment<
            true,
            RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
            RHI_BLEND_FACTOR_ONE,
            RHI_BLEND_OP_MULTIPLY,
            RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
            RHI_BLEND_FACTOR_ONE,
            RHI_BLEND_OP_MULTIPLY>>();
    }

    set_blend_state(default_blend_state);

    set_surface_type(SURFACE_TYPE_TRANSPARENT);
}

void gf2_material_eye_blend::set_blend(const texture_2d* texture)
{
    get_constant().blend_texture = texture->get_srv()->get_bindless();
}

rhi_shader* gf2_material_eye_blend::get_vertex_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_vs>(defines);
}

rhi_shader* gf2_material_eye_blend::get_fragment_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_eye_blend_fs>(defines);
}

gf2_material_hair::gf2_material_hair()
{
    auto& device = render_device::instance();

    set_cull_mode(RHI_CULL_MODE_BACK);
    set_shading_model<toon_shading_model>();
    set_surface_type(SURFACE_TYPE_OPAQUE);

    get_constant().brdf_lut = device.get_texture<brdf_lut>()->get_srv()->get_bindless();
}

void gf2_material_hair::set_diffuse(const texture_2d* texture)
{
    get_constant().diffuse_texture = texture->get_srv()->get_bindless();
}

void gf2_material_hair::set_specular(const texture_2d* texture)
{
    get_constant().specular_texture = texture->get_srv()->get_bindless();
}

void gf2_material_hair::set_ramp(const texture_2d* texture)
{
    get_constant().ramp_texture = texture->get_srv()->get_bindless();
}

rhi_shader* gf2_material_hair::get_vertex_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_vs>(defines);
}

rhi_shader* gf2_material_hair::get_fragment_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_hair_fs>(defines);
}

gf2_material_plush::gf2_material_plush()
{
    auto& device = render_device::instance();

    set_cull_mode(RHI_CULL_MODE_BACK);
    set_shading_model<toon_shading_model>();
    set_surface_type(SURFACE_TYPE_OPAQUE);

    get_constant().brdf_lut = device.get_texture<brdf_lut>()->get_srv()->get_bindless();
}

void gf2_material_plush::set_diffuse(const texture_2d* texture)
{
    get_constant().diffuse_texture = texture->get_srv()->get_bindless();
}

void gf2_material_plush::set_normal(const texture_2d* texture)
{
    get_constant().normal_texture = texture->get_srv()->get_bindless();
}

void gf2_material_plush::set_noise(const texture_2d* texture)
{
    get_constant().noise_texture = texture->get_srv()->get_bindless();
}

void gf2_material_plush::set_ramp(const texture_2d* texture)
{
    get_constant().ramp_texture = texture->get_srv()->get_bindless();
}

rhi_shader* gf2_material_plush::get_vertex_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_vs>(defines);
}

rhi_shader* gf2_material_plush::get_fragment_shader(std::span<std::wstring> defines) const
{
    return render_device::instance().get_shader<gf2_material_plush_fs>(defines);
}
} // namespace violet