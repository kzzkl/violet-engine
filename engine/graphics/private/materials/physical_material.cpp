#include "graphics/materials/physical_material.hpp"

namespace violet
{
enum physical_use_texture
{
    PHYSICAL_USE_TEXTURE_OFF,
    PHYSICAL_USE_TEXTURE_ON
};

struct physical_material_vs : public vertex_shader
{
    using options = shader::options<physical_use_texture>;

    static std::string_view get_path(const options& options)
    {
        if (options.get<physical_use_texture>() == PHYSICAL_USE_TEXTURE_OFF)
            return "engine/shaders/physical.vs";
        else
            return "engine/shaders/physical_tex.vs";
    }

    static parameter_slots get_parameters(const options& options)
    {
        return {
            {0, camera},
            {2, mesh  }
        };
    }

    static input_slots get_inputs(const options& options)
    {
        if (options.get<physical_use_texture>() == PHYSICAL_USE_TEXTURE_OFF)
        {
            return {
                {"position", RHI_FORMAT_R32G32B32_FLOAT},
                {"normal",   RHI_FORMAT_R32G32B32_FLOAT}
            };
        }
        else
        {
            return {
                {"position", RHI_FORMAT_R32G32B32_FLOAT},
                {"normal",   RHI_FORMAT_R32G32B32_FLOAT},
                {"tangent",  RHI_FORMAT_R32G32B32_FLOAT},
                {"uv",       RHI_FORMAT_R32G32_FLOAT   }
            };
        }
    }
};

struct physical_material_fs : public fragment_shader
{
    using options = shader_options<physical_use_texture>;

    static std::string_view get_path(const options& options)
    {
        if (options.get<physical_use_texture>() == PHYSICAL_USE_TEXTURE_OFF)
            return "engine/shaders/physical.fs";
        else
            return "engine/shaders/physical_tex.fs";
    }

    static constexpr parameter material = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                              },
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                              },
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                              },
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(physical_material::data)}
    };

    static constexpr parameter material_tex = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}
    };

    static parameter_slots get_parameters(const options& options)
    {
        if (options.get<physical_use_texture>() == PHYSICAL_USE_TEXTURE_OFF)
        {
            return {
                {0, camera  },
                {1, light   },
                {3, material}
            };
        }
        else
        {
            return {
                {0, camera      },
                {1, light       },
                {2, mesh        },
                {3, material_tex}
            };
        }
    }
};

physical_material::physical_material(bool use_texture) : m_use_texture(use_texture)
{
    physical_material_vs::options vs_options = {};
    vs_options.set(use_texture ? PHYSICAL_USE_TEXTURE_ON : PHYSICAL_USE_TEXTURE_OFF);

    physical_material_fs::options fs_options = {};
    fs_options.set(use_texture ? PHYSICAL_USE_TEXTURE_ON : PHYSICAL_USE_TEXTURE_OFF);

    rdg_render_pipeline pipeline = {};
    pipeline.vertex_shader = render_device::instance().get_shader<physical_material_vs>(vs_options);
    pipeline.fragment_shader =
        render_device::instance().get_shader<physical_material_fs>(fs_options);
    if (m_use_texture)
        add_pass(pipeline, physical_material_fs::material_tex);
    else
        add_pass(pipeline, physical_material_fs::material);
}

void physical_material::set_albedo(const float3& albedo)
{
    assert(!m_use_texture);
    get_parameter(0)->set_uniform(3, &albedo, sizeof(float3), offsetof(data, albedo));
}

void physical_material::set_roughness(float roughness)
{
    assert(!m_use_texture);
    get_parameter(0)->set_uniform(3, &roughness, sizeof(float), offsetof(data, roughness));
}

void physical_material::set_metallic(float metallic)
{
    assert(!m_use_texture);
    get_parameter(0)->set_uniform(3, &metallic, sizeof(float), offsetof(data, metallic));
}

void physical_material::set_albedo(rhi_texture* albedo, rhi_sampler* sampler)
{
    assert(m_use_texture);
    get_parameter(0)->set_texture(3, albedo, sampler);
}

void physical_material::set_roughness(rhi_texture* roughness, rhi_sampler* sampler)
{
    assert(m_use_texture);
    get_parameter(0)->set_texture(4, roughness, sampler);
}

void physical_material::set_metallic(rhi_texture* metallic, rhi_sampler* sampler)
{
    assert(m_use_texture);
    get_parameter(0)->set_texture(5, metallic, sampler);
}

void physical_material::set_normal(rhi_texture* normal, rhi_sampler* sampler)
{
    assert(m_use_texture);
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