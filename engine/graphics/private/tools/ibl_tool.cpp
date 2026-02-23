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
        std::uint32_t env_map;
        std::uint32_t cube_map;
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
        std::uint32_t cube_map;
        std::uint32_t irradiance_map;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/prefilter.hlsl";

    struct constant_data
    {
        std::uint32_t cube_map;
        std::uint32_t prefilter_map;
        float roughness;
        std::uint32_t resolution;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
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

private:
    static void add_convert_pass(render_graph& graph, rdg_texture* env_map, rdg_texture* cube_map)
    {
        struct pass_data
        {
            rdg_texture_srv env_map;
            rdg_texture_uav cube_map;
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
            },
            [](const pass_data& data, rdg_command& command)
            {
                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<convert_cs>(),
                });
                command.set_constant(
                    convert_cs::constant_data{
                        .env_map = data.env_map.get_bindless(),
                        .cube_map = data.cube_map.get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_texture_extent extent = data.cube_map.get_texture()->get_extent();
                command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
            });
    }

    static void add_mipmap_pass(render_graph& graph, rdg_texture* cube_map)
    {
        struct pass_data
        {
            rdg_texture_ref src;
            rdg_texture_ref dst;
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
                },
                [](const pass_data& data, rdg_command& command)
                {
                    rhi_texture_region src_region = {
                        .extent = data.src.get_extent(),
                        .level = data.src.get_level(),
                        .layer = data.src.get_layer(),
                        .layer_count = data.src.get_layer_count(),
                    };

                    rhi_texture_region dst_region = {
                        .extent = data.dst.get_extent(),
                        .level = data.dst.get_level(),
                        .layer = data.dst.get_layer(),
                        .layer_count = data.dst.get_layer_count(),
                    };

                    command.blit_texture(
                        data.src.get_rhi(),
                        src_region,
                        data.dst.get_rhi(),
                        dst_region,
                        RHI_FILTER_LINEAR);
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
            },
            [](const pass_data& data, rdg_command& command)
            {
                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<irradiance_cs>(),
                });
                command.set_constant(
                    irradiance_cs::constant_data{
                        .cube_map = data.cube_map.get_bindless(),
                        .irradiance_map = data.irradiance_map.get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_texture_extent extent = data.irradiance_map.get_texture()->get_extent();
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
                },
                [](const pass_data& data, rdg_command& command)
                {
                    std::uint32_t level = data.prefilter_map.get_level();
                    std::uint32_t level_count = data.prefilter_map.get_texture()->get_level_count();

                    command.set_pipeline({
                        .compute_shader = render_device::instance().get_shader<prefilter_cs>(),
                    });
                    command.set_constant(
                        prefilter_cs::constant_data{
                            .cube_map = data.cube_map.get_bindless(),
                            .prefilter_map = data.prefilter_map.get_bindless(),
                            .roughness =
                                static_cast<float>(level) / static_cast<float>(level_count),
                            .resolution = data.cube_map.get_texture()->get_extent().width,
                        });
                    command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                    rhi_texture_extent extent = data.prefilter_map.get_extent();
                    command.dispatch_3d(extent.width, extent.height, 6, 8, 8, 1);
                });
        }
    }
};

void ibl_tool::generate_cube_map(rhi_texture* env_map, rhi_texture* cube_map)
{
    render_graph graph("Generate Cube Map");

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
    render_graph graph("Generate IBL");

    ibl_renderer::render_ibl(graph, cube_map, irradiance_map, prefilter_map);

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);

    device.execute_sync(command);
}
} // namespace violet