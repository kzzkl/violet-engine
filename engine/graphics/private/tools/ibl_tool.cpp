#include "tools/ibl_tool.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct convert_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/hdri_to_cubemap.hlsl";

    struct constant_data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t env_map;
        std::uint32_t cube_map;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(constant_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

struct irradiance_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/irradiance.hlsl";

    struct constant_data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t cube_map;
        std::uint32_t irradiance_map;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(constant_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

struct prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/prefilter.hlsl";

    struct constant_data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t cube_map;
        std::uint32_t prefilter_map;
        float roughness;
        std::uint32_t resolution;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(constant_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

struct brdf_lut_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/brdf_lut.hlsl";

    struct constant_data
    {
        std::uint32_t brdf_lut;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t padding0;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(constant_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

class ibl_renderer
{
public:
    static void render_cube_map(render_graph& graph, rhi_texture* env_map, rhi_texture* cube_map)
    {
        rdg_scope scope(graph, "IBL Generate");

        rdg_texture* env = graph.add_texture(
            "Environment Map",
            env_map,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        rdg_texture* cube = graph.add_texture(
            "Cube Map",
            cube_map,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        add_convert_pass(graph, env, cube);
        add_mipmap_pass(graph, cube);
    }

    static void render_ibl(
        render_graph& graph,
        rhi_texture* cube_map,
        rhi_texture* irradiance_map,
        rhi_texture* prefilter_map)
    {
        rdg_scope scope(graph, "IBL Generate");

        rdg_texture* cube = graph.add_texture(
            "Cube Map",
            cube_map,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        {
            rdg_scope scope(graph, "Irradiance");

            rdg_texture* irradiance = graph.add_texture(
                "Irradiance Map",
                irradiance_map,
                RHI_TEXTURE_LAYOUT_UNDEFINED,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

            add_irradiance_pass(graph, cube, irradiance);
        }

        {
            rdg_scope scope(graph, "Prefilter");

            rdg_texture* prefilter = graph.add_texture(
                "Prefilter Map",
                prefilter_map,
                RHI_TEXTURE_LAYOUT_UNDEFINED,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

            add_prefilter_pass(graph, cube, prefilter);
        }
    }

    static void render_brdf_lut(render_graph& graph, rhi_texture* brdf_lut)
    {
        add_brdf_pass(graph, brdf_lut);
    }

private:
    static void add_convert_pass(render_graph& graph, rdg_texture* env_map, rdg_texture* cube_map)
    {
        struct pass_data
        {
            rdg_texture_srv env_map;
            rdg_texture_uav cube_map;
            rhi_parameter* parameter;
        };

        graph.add_pass<pass_data>(
            "HDRI Convert Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.env_map = pass.add_texture_srv(env_map, RHI_PIPELINE_STAGE_COMPUTE);
                data.cube_map = pass.add_texture_uav(
                    cube_map,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D_ARRAY);
                data.parameter = pass.add_parameter(convert_cs::parameter);
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_extent extent = data.cube_map.get_texture()->get_extent();

                convert_cs::constant_data constant = {
                    .width = extent.width,
                    .height = extent.height,
                    .env_map = data.env_map.get_bindless(),
                    .cube_map = data.cube_map.get_bindless(),
                };
                data.parameter->set_constant(0, &constant, sizeof(convert_cs::constant_data));

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<convert_cs>(),
                });
                command.set_parameter(0, render_device::instance().get_bindless_parameter());
                command.set_parameter(1, data.parameter);
                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }

    static void add_mipmap_pass(render_graph& graph, rdg_texture* cube_map)
    {
        struct pass_data
        {
            rdg_texture_ref src;
            rdg_texture_ref dst;

            std::uint32_t level;
        };

        for (std::uint32_t level = 1; level < cube_map->get_level_count(); ++level)
        {
            graph.add_pass<pass_data>(
                "Mipmap Pass " + std::to_string(level),
                RDG_PASS_TRANSFER,
                [&](pass_data& data, rdg_pass& pass)
                {
                    data.src = pass.add_texture(
                        cube_map,
                        RHI_PIPELINE_STAGE_TRANSFER,
                        RHI_ACCESS_TRANSFER_READ,
                        RHI_TEXTURE_LAYOUT_TRANSFER_SRC,
                        level - 1,
                        1);

                    data.dst = pass.add_texture(
                        cube_map,
                        RHI_PIPELINE_STAGE_TRANSFER,
                        RHI_ACCESS_TRANSFER_WRITE,
                        RHI_TEXTURE_LAYOUT_TRANSFER_DST,
                        level,
                        1);

                    data.level = level;
                },
                [](const pass_data& data, rdg_command& command)
                {
                    rhi_texture_region src_region = {
                        .extent = data.src.get_extent(),
                        .level = data.level - 1,
                        .layer = 0,
                        .layer_count = 6,
                    };

                    rhi_texture_region dst_region = {
                        .extent = data.dst.get_extent(),
                        .level = data.level,
                        .layer = 0,
                        .layer_count = 6,
                    };

                    command.blit_texture(
                        data.src.get_rhi(),
                        src_region,
                        data.dst.get_rhi(),
                        dst_region);
                });
        }
    }

    static void add_irradiance_pass(
        render_graph& graph,
        rdg_texture* cube_map,
        rdg_texture* irradiance_map)
    {
        struct pass_data
        {
            rdg_texture_srv cube_map;
            rdg_texture_uav irradiance_map;
            rhi_parameter* parameter;
        };

        graph.add_pass<pass_data>(
            "Irradiance Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.cube_map = pass.add_texture_srv(
                    cube_map,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_CUBE);
                data.irradiance_map = pass.add_texture_uav(
                    irradiance_map,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D_ARRAY);

                data.parameter = pass.add_parameter(irradiance_cs::parameter);
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_extent extent = data.irradiance_map.get_texture()->get_extent();

                irradiance_cs::constant_data irradiance_constant = {
                    .width = extent.width,
                    .height = extent.height,
                    .cube_map = data.cube_map.get_bindless(),
                    .irradiance_map = data.irradiance_map.get_bindless(),
                };
                data.parameter->set_constant(
                    0,
                    &irradiance_constant,
                    sizeof(irradiance_cs::constant_data));

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<irradiance_cs>(),
                });
                command.set_parameter(0, render_device::instance().get_bindless_parameter());
                command.set_parameter(1, data.parameter);
                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }

    static void add_prefilter_pass(
        render_graph& graph,
        rdg_texture* cube_map,
        rdg_texture* prefilter_map)
    {
        struct pass_data
        {
            rdg_texture_srv cube_map;
            rdg_texture_uav prefilter_map;
            rhi_parameter* parameter;
            std::uint32_t level;
        };

        for (std::uint32_t level = 0; level < prefilter_map->get_level_count(); ++level)
        {
            graph.add_pass<pass_data>(
                "Prefilter Pass Mip " + std::to_string(level),
                RDG_PASS_COMPUTE,
                [&](pass_data& data, rdg_pass& pass)
                {
                    data.cube_map = pass.add_texture_srv(
                        cube_map,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_CUBE);
                    data.prefilter_map = pass.add_texture_uav(
                        prefilter_map,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_2D_ARRAY,
                        level,
                        1);

                    data.parameter = pass.add_parameter(prefilter_cs::parameter);
                    data.level = level;
                },
                [](const pass_data& data, rdg_command& command)
                {
                    rhi_texture_extent extent = data.prefilter_map.get_extent();
                    std::uint32_t level_count = data.prefilter_map.get_texture()->get_level_count();

                    prefilter_cs::constant_data constant = {
                        .width = extent.width,
                        .height = extent.height,
                        .cube_map = data.cube_map.get_bindless(),
                        .prefilter_map = data.prefilter_map.get_bindless(),
                        .roughness =
                            static_cast<float>(data.level) / static_cast<float>(level_count),
                        .resolution = data.cube_map.get_texture()->get_extent().width,
                    };
                    data.parameter->set_constant(0, &constant, sizeof(prefilter_cs::constant_data));

                    command.set_pipeline({
                        .compute_shader = render_device::instance().get_shader<prefilter_cs>(),
                    });
                    command.set_parameter(0, render_device::instance().get_bindless_parameter());
                    command.set_parameter(1, data.parameter);
                    command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
                });
        }
    }

    static void add_brdf_pass(render_graph& graph, rhi_texture* brdf_map)
    {
        rdg_texture* brdf_lut = graph.add_texture(
            "BRDF LUT",
            brdf_map,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        struct pass_data
        {
            rdg_texture_uav brdf_lut;
            rhi_parameter* parameter;
        };

        graph.add_pass<pass_data>(
            "BRDF LUT Pass",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.brdf_lut = pass.add_texture_uav(brdf_lut, RHI_PIPELINE_STAGE_COMPUTE);
                data.parameter = pass.add_parameter(brdf_lut_cs::parameter);
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_extent extent = data.brdf_lut.get_texture()->get_extent();

                brdf_lut_cs::constant_data brdf_lut_constant = {
                    .brdf_lut = data.brdf_lut.get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                };
                data.parameter->set_constant(
                    0,
                    &brdf_lut_constant,
                    sizeof(brdf_lut_cs::constant_data));

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<brdf_lut_cs>(),
                });

                command.set_parameter(0, render_device::instance().get_bindless_parameter());
                command.set_parameter(1, data.parameter);

                command.dispatch_2d(extent.width, extent.height);
            });
    }
};

void ibl_tool::generate_cube_map(rhi_texture* env_map, rhi_texture* cube_map)
{
    rdg_allocator allocator;
    render_graph graph("Generate Cube Map", &allocator);

    ibl_renderer::render_cube_map(graph, env_map, cube_map);

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);

    device.execute_sync(command);
}

void ibl_tool::generate_ibl(
    rhi_texture* cube_map,
    rhi_texture* irradiance_map,
    rhi_texture* prefilter_map)
{
    rdg_allocator allocator;
    render_graph graph("Generate IBL", &allocator);

    ibl_renderer::render_ibl(graph, cube_map, irradiance_map, prefilter_map);

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);

    device.execute_sync(command);
}

void ibl_tool::generate_brdf_lut(rhi_texture* brdf_lut)
{
    rdg_allocator allocator;
    render_graph graph("Generate BRDF LUT", &allocator);

    ibl_renderer::render_brdf_lut(graph, brdf_lut);

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);

    device.execute_sync(command);
}
} // namespace violet