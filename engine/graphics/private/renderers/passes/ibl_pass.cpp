#include "graphics/renderers/passes/ibl_pass.hpp"

namespace violet
{
struct prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/prefilter.hlsl";

    struct constant_data
    {
        std::uint32_t environment_map;
        std::uint32_t prefilter_map;
        float roughness;
        std::uint32_t resolution;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct irradiance_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/irradiance.hlsl";

    struct constant_data
    {
        std::uint32_t environment_map;
        std::uint32_t buffer_size;
        std::uint32_t irradiance_sh_src;
        std::uint32_t irradiance_sh_dst;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void prefilter_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv environment_map;
        rdg_texture_uav prefilter_map;
    };

    for (std::uint32_t level = 0; level < parameter.prefilter_map->get_level_count(); ++level)
    {
        graph.add_pass<pass_data>(
            "Prefilter Pass Mip " + std::to_string(level),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.environment_map = pass.add_texture_srv(
                    parameter.environment_map,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_CUBE);
                data.prefilter_map = pass.add_texture_uav(
                    parameter.prefilter_map,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D_ARRAY,
                    level,
                    1);
            },
            [](const pass_data& data, rdg_command& command)
            {
                std::uint32_t level = data.prefilter_map.get_level();
                std::uint32_t level_count = data.prefilter_map.get_texture()->get_level_count();

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<prefilter_cs>(),
                });

                rhi_extent extent = data.prefilter_map.get_extent();
                command.set_constant(
                    prefilter_cs::constant_data{
                        .environment_map = data.environment_map.get_bindless(),
                        .prefilter_map = data.prefilter_map.get_bindless(),
                        .roughness = static_cast<float>(level) / static_cast<float>(level_count),
                        .resolution = data.environment_map.get_texture()->get_extent().width,
                        .width = extent.width,
                        .height = extent.height,
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }
}

void irradiance_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv environment_map;
        rdg_buffer_srv irradiance_sh_src;
        rdg_buffer_uav irradiance_sh_dst;
    };

    std::uint32_t width = parameter.environment_map->get_extent().width;
    assert(width >= 8);

    rdg_buffer* irradiance_sh_src = graph.add_buffer(
        "Irradiance SH Temp 0",
        sizeof(vec4f) * 9 * (width / 8) * (width / 8),
        RHI_BUFFER_STORAGE);
    rdg_buffer* irradiance_sh_dst = graph.add_buffer(
        "Irradiance SH Temp 1",
        sizeof(vec4f) * 9 * (width / 8) * (width / 8),
        RHI_BUFFER_STORAGE);

    while (width > 0)
    {
        bool main_pass = width == parameter.environment_map->get_extent().width;

        if (width <= 8)
        {
            irradiance_sh_dst = parameter.irradiance_sh;
        }

        graph.add_pass<pass_data>(
            "Irradiance Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                if (main_pass)
                {
                    data.environment_map = pass.add_texture_srv(
                        parameter.environment_map,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_CUBE);
                }
                else
                {
                    data.irradiance_sh_src =
                        pass.add_buffer_srv(irradiance_sh_src, RHI_PIPELINE_STAGE_COMPUTE);
                }

                data.irradiance_sh_dst =
                    pass.add_buffer_uav(irradiance_sh_dst, RHI_PIPELINE_STAGE_COMPUTE);
            },
            [=](const pass_data& data, rdg_command& command)
            {
                std::vector<std::wstring> defines;

                irradiance_cs::constant_data constant_data = {
                    .buffer_size = width * width,
                    .irradiance_sh_dst = data.irradiance_sh_dst.get_bindless(),
                };

                if (main_pass)
                {
                    defines.emplace_back(L"-DIRRADIANCE_SH_STAGE_MAIN");
                    constant_data.environment_map = data.environment_map.get_bindless();
                }
                else
                {
                    constant_data.irradiance_sh_src = data.irradiance_sh_src.get_bindless();
                }

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<irradiance_cs>(defines),
                });
                command.set_constant(constant_data);
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command.dispatch_1d(width * width);
            });

        width /= 8;
        std::swap(irradiance_sh_src, irradiance_sh_dst);
    }
}
} // namespace violet