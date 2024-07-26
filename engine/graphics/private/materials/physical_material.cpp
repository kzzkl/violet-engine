#include "graphics/materials/physical_material.hpp"

namespace violet
{
struct physical_material_vs : public vertex_shader<physical_material_vs>
{
    static constexpr std::string_view path = "engine/shaders/physical.vs";
    static constexpr parameter_slot parameters[] = {
        {0, camera},
        {2, mesh  }
    };

    static constexpr input inputs[] = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal",   RHI_FORMAT_R32G32B32_FLOAT}
    };
};

struct physical_material_fs : public fragment_shader<physical_material_fs>
{
    static constexpr std::string_view path = "engine/shaders/physical.fs";

    static constexpr parameter material = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                              },
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                              },
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                              },
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(physical_material::data)}
    };

    static constexpr parameter_slot parameters[] = {
        {0, camera  },
        {1, light   },
        {3, material}
    };
};

struct physical_material_tex_vs : public vertex_shader<physical_material_tex_vs>
{
    static constexpr std::string_view path = "engine/shaders/physical_tex.vs";
    static constexpr parameter_slot parameters[] = {
        {0, camera},
        {2, mesh  }
    };

    static constexpr input inputs[] = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal",   RHI_FORMAT_R32G32B32_FLOAT},
        {"tangent",   RHI_FORMAT_R32G32B32_FLOAT},
        {"uv",       RHI_FORMAT_R32G32_FLOAT   }
    };
};

struct physical_material_tex_fs : public fragment_shader<physical_material_tex_fs>
{
    static constexpr std::string_view path = "engine/shaders/physical_tex.fs";

    static constexpr parameter material = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}
    };

    static constexpr parameter_slot parameters[] = {
        {0, camera  },
        {1, light   },
        {2, mesh    },
        {3, material}
    };
};

physical_material::physical_material(bool texture) : m_texture(texture)
{
    if (m_texture)
    {
        rdg_render_pipeline pipeline = {};
        pipeline.vertex_shader = physical_material_tex_vs::get_rhi();
        pipeline.fragment_shader = physical_material_tex_fs::get_rhi();
        add_pass(pipeline, physical_material_tex_fs::material);
    }
    else
    {
        rdg_render_pipeline pipeline = {};
        pipeline.vertex_shader = physical_material_vs::get_rhi();
        pipeline.fragment_shader = physical_material_fs::get_rhi();
        add_pass(pipeline, physical_material_fs::material);
    }
}

void physical_material::set_albedo(const float3& albedo)
{
    assert(!m_texture);
    get_parameter(0)->set_uniform(3, &albedo, sizeof(float3), offsetof(data, albedo));
}

void physical_material::set_roughness(float roughness)
{
    assert(!m_texture);
    get_parameter(0)->set_uniform(3, &roughness, sizeof(float), offsetof(data, roughness));
}

void physical_material::set_metallic(float metallic)
{
    assert(!m_texture);
    get_parameter(0)->set_uniform(3, &metallic, sizeof(float), offsetof(data, metallic));
}

void physical_material::set_albedo(rhi_texture* albedo, rhi_sampler* sampler)
{
    assert(m_texture);
    get_parameter(0)->set_texture(3, albedo, sampler);
}

void physical_material::set_roughness(rhi_texture* roughness, rhi_sampler* sampler)
{
    assert(m_texture);
    get_parameter(0)->set_texture(4, roughness, sampler);
}

void physical_material::set_metallic(rhi_texture* metallic, rhi_sampler* sampler)
{
    assert(m_texture);
    get_parameter(0)->set_texture(5, metallic, sampler);
}

void physical_material::set_normal(rhi_texture* normal, rhi_sampler* sampler)
{
    assert(m_texture);
    get_parameter(0)->set_texture(6, normal, sampler);
}

void physical_material::set_irradiance_map(rhi_texture* irradiance_map, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(0, irradiance_map, sampler);
}

void physical_material::set_prefilter_map(rhi_texture* prefilter_map, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(1, prefilter_map, sampler);
}

void physical_material::set_brdf_lut(rhi_texture* brdf_lut, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(2, brdf_lut, sampler);
}
} // namespace violet