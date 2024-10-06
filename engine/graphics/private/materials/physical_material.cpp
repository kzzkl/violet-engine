#include "graphics/materials/physical_material.hpp"

namespace violet
{
template <bool use_uv, bool use_tangent>
struct physical_material_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/source/materials/physical_tex.hlsl";

    static constexpr parameter_layout parameters = {{0, camera}, {2, mesh}};

    static constexpr input_layout inputs = []()
    {
        input_layout layout = {};
        layout.attributes[0] = {"position", RHI_FORMAT_R32G32B32_FLOAT};
        layout.attributes[1] = {"normal", RHI_FORMAT_R32G32B32_FLOAT};
        layout.attribute_count = 2;

        if constexpr (use_tangent)
        {
            layout.attributes[layout.attribute_count] = {"tangent", RHI_FORMAT_R32G32B32_FLOAT};
            ++layout.attribute_count;
        }

        if constexpr (use_uv)
        {
            layout.attributes[layout.attribute_count] = {"uv", RHI_FORMAT_R32G32_FLOAT};
            ++layout.attribute_count;
        }

        return layout;
    }();
};

struct physical_material_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/source/materials/physical_tex.hlsl";

    static constexpr parameter material = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(physical_material::data)}};

    static constexpr parameter material_tex = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}};

    static constexpr parameter_layout parameters = {{0, camera}, {1, light}, {3, material_tex}};
};

physical_material::physical_material()
{
    rdg_render_pipeline pipeline = {};
    pipeline.vertex_shader =
        render_device::instance().get_shader<physical_material_vs<true, true>>();
    pipeline.fragment_shader = render_device::instance().get_shader<physical_material_fs>();
    add_pass(pipeline, physical_material_fs::material_tex);
}

void physical_material::set_albedo(const float3& albedo)
{
    get_passes()[0].parameter->set_uniform(3, &albedo, sizeof(float3), offsetof(data, albedo));
}

void physical_material::set_roughness(float roughness)
{
    get_passes()[0].parameter->set_uniform(3, &roughness, sizeof(float), offsetof(data, roughness));
}

void physical_material::set_metallic(float metallic)
{
    get_passes()[0].parameter->set_uniform(3, &metallic, sizeof(float), offsetof(data, metallic));
}

void physical_material::set_albedo(rhi_texture* albedo, rhi_sampler* sampler)
{
    get_passes()[0].parameter->set_texture(3, albedo, sampler);
}

void physical_material::set_roughness(rhi_texture* roughness, rhi_sampler* sampler)
{
    get_passes()[0].parameter->set_texture(4, roughness, sampler);
}

void physical_material::set_metallic(rhi_texture* metallic, rhi_sampler* sampler)
{
    get_passes()[0].parameter->set_texture(5, metallic, sampler);
}

void physical_material::set_normal(rhi_texture* normal, rhi_sampler* sampler)
{
    auto& pass = get_pass(0);
    pass.pipeline.vertex_shader =
        render_device::instance().get_shader<physical_material_vs<true, true>>();
    pass.parameter->set_texture(6, normal, sampler);
}

void physical_material::set_irradiance_map(rhi_texture* irradiance_map, rhi_sampler* sampler)
{
    get_passes()[0].parameter->set_texture(0, irradiance_map, sampler);
}

void physical_material::set_prefilter_map(rhi_texture* prefilter_map, rhi_sampler* sampler)
{
    get_passes()[0].parameter->set_texture(1, prefilter_map, sampler);
}

void physical_material::set_brdf_lut(rhi_texture* brdf_lut, rhi_sampler* sampler)
{
    get_passes()[0].parameter->set_texture(2, brdf_lut, sampler);
}

void physical_material::update_pso()
{
    auto use_texture = [this]()
    {
        return m_abledo_texture || m_roughness_texture || m_metallic_texture || m_normal_texture;
    };

    std::vector<std::wstring> defines;
    if (use_texture())
    {
        defines.push_back(L"USE_TEXTURE");
    }

    auto& pass = get_pass(0);

    if (use_texture())
    {
        if (m_normal_texture)
        {
            pass.pipeline.vertex_shader =
                render_device::instance().get_shader<physical_material_vs<true, true>>();
        }
        else
        {
            pass.pipeline.vertex_shader =
                render_device::instance().get_shader<physical_material_vs<true, false>>();
        }
    }
    else
    {
        pass.pipeline.vertex_shader =
            render_device::instance().get_shader<physical_material_vs<false, false>>();
    }
}
} // namespace violet