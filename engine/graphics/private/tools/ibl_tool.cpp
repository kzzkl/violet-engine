#include "graphics/tools/ibl_tool.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct convert_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/hdri_to_cubemap.hlsl";

    struct convert_data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t env_map;
        std::uint32_t cube_map;
    };

    static constexpr parameter convert = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(convert_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, convert},
    };
};

struct irradiance_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/irradiance.hlsl";

    struct irradiance_data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t cube_map;
        std::uint32_t irradiance_map;
    };

    static constexpr parameter irradiance = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(irradiance_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, irradiance},
    };
};

struct prefilter_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/ibl/prefilter.hlsl";

    struct prefilter_data
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t cube_map;
        std::uint32_t prefilter_map;
        float roughness;
        std::uint32_t level;
        std::uint32_t padding0;
        std::uint32_t padding1;
    };

    static constexpr parameter prefilter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_COMPUTE,
            .size = sizeof(prefilter_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, prefilter},
    };
};

struct brdf_lut_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/ibl/brdf_lut.hlsl";
};

class ibl_renderer
{
public:
    static void render_cube_map(render_graph& graph, rhi_texture* env_map, rhi_texture* cube_map)
    {
        rdg_scope scope(graph, "IBL Generate");

        rhi_ptr<rhi_texture> cube_map_view = render_device::instance().create_texture({
            .type = RHI_TEXTURE_VIEW_2D_ARRAY,
            .texture = cube_map,
            .level = 0,
            .level_count = 1,
            .layer = 0,
            .layer_count = 6,
        });

        rdg_texture* env = graph.add_texture(
            "Environment Map",
            env_map,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        rdg_texture* cube = graph.add_texture(
            "Cube Map",
            cube_map_view.get(),
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        add_convert_pass(graph, env, cube);
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

            rhi_ptr<rhi_texture> irradiance_map_view = render_device::instance().create_texture({
                .type = RHI_TEXTURE_VIEW_2D_ARRAY,
                .texture = irradiance_map,
                .level = 0,
                .level_count = 1,
                .layer = 0,
                .layer_count = 6,
            });

            rdg_texture* irradiance = graph.add_texture(
                "Irradiance Map",
                irradiance_map_view.get(),
                RHI_TEXTURE_LAYOUT_UNDEFINED,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

            add_irradiance_pass(graph, cube, irradiance);
        }

        {
            rdg_scope scope(graph, "Prefilter");

            std::uint32_t level_count = prefilter_map->get_level_count();
            for (std::uint32_t i = 0; i < level_count; ++i)
            {
                rdg_scope scope(graph, "Mip " + std::to_string(i));

                rhi_ptr<rhi_texture> prefilter_map_view = render_device::instance().create_texture({
                    .type = RHI_TEXTURE_VIEW_2D_ARRAY,
                    .texture = prefilter_map,
                    .level = i,
                    .level_count = 1,
                    .layer = 0,
                    .layer_count = 6,
                });

                rdg_texture* prefilter = graph.add_texture(
                    "Prefilter Map Mip " + std::to_string(i),
                    prefilter_map_view.get(),
                    RHI_TEXTURE_LAYOUT_UNDEFINED,
                    RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

                add_prefilter_pass(
                    graph,
                    cube,
                    prefilter,
                    static_cast<float>(i) / static_cast<float>(level_count),
                    i);
            }
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
            rhi_texture_extent extent;
            rdg_texture* env_map;
            rdg_texture* cube_map;
            rhi_parameter* parameter;
        };

        pass_data data = {
            .extent = cube_map->get_extent(),
            .env_map = env_map,
            .cube_map = cube_map,
            .parameter = graph.allocate_parameter(convert_cs::convert),
        };
        auto& pass = graph.add_pass<rdg_compute_pass>("Convert Pass");
        pass.add_texture(
            env_map,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_READ,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        pass.add_texture(
            cube_map,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_GENERAL);
        pass.set_execute(
            [data](rdg_command& command)
            {
                convert_cs::convert_data convert_constant = {
                    .width = data.extent.width,
                    .height = data.extent.height,
                    .env_map = data.env_map->get_handle(),
                    .cube_map = data.cube_map->get_handle(),
                };
                data.parameter->set_constant(0, &convert_constant, sizeof(convert_constant));

                rdg_compute_pipeline pipeline = {
                    .compute_shader = render_device::instance().get_shader<convert_cs>(),
                };

                command.set_pipeline(pipeline);
                command.set_parameter(0, render_device::instance().get_bindless_parameter());
                command.set_parameter(1, data.parameter);
                command.dispatch_3d(data.extent.width, data.extent.height, 6, 8, 8, 1);
            });
    }

    static void add_irradiance_pass(
        render_graph& graph,
        rdg_texture* cube_map,
        rdg_texture* irradiance_map)
    {
        struct pass_data
        {
            rhi_texture_extent extent;
            rdg_texture* cube_map;
            rdg_texture* irradiance_map;
            rhi_parameter* parameter;
        };

        pass_data data = {
            .extent = irradiance_map->get_extent(),
            .cube_map = cube_map,
            .irradiance_map = irradiance_map,
            .parameter = graph.allocate_parameter(irradiance_cs::irradiance),
        };

        auto& pass = graph.add_pass<rdg_compute_pass>("Irradiance Pass");
        pass.add_texture(
            cube_map,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_READ,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        pass.add_texture(
            irradiance_map,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_GENERAL);
        pass.set_execute(
            [data](rdg_command& command)
            {
                irradiance_cs::irradiance_data irradiance_constant = {
                    .width = data.extent.width,
                    .height = data.extent.height,
                    .cube_map = data.cube_map->get_handle(),
                    .irradiance_map = data.irradiance_map->get_handle(),
                };
                data.parameter->set_constant(0, &irradiance_constant, sizeof(irradiance_constant));

                rdg_compute_pipeline pipeline = {
                    .compute_shader = render_device::instance().get_shader<irradiance_cs>(),
                };

                command.set_pipeline(pipeline);
                command.set_parameter(0, render_device::instance().get_bindless_parameter());
                command.set_parameter(1, data.parameter);
                command.dispatch_3d(data.extent.width, data.extent.height, 6, 8, 8, 1);
            });
    }

    static void add_prefilter_pass(
        render_graph& graph,
        rdg_texture* cube_map,
        rdg_texture* prefilter_map,
        float roughness,
        std::uint32_t level)
    {
        struct pass_data
        {
            rhi_texture_extent extent;
            rdg_texture* cube_map;
            rdg_texture* prefilter_map;
            rhi_parameter* parameter;
        };

        pass_data data = {
            .extent = prefilter_map->get_extent(),
            .cube_map = cube_map,
            .prefilter_map = prefilter_map,
            .parameter = graph.allocate_parameter(prefilter_cs::prefilter),
        };

        auto& pass = graph.add_pass<rdg_compute_pass>("Prefilter Pass");
        pass.add_texture(
            cube_map,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_READ,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        pass.add_texture(
            prefilter_map,
            RHI_PIPELINE_STAGE_COMPUTE,
            RHI_ACCESS_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_GENERAL);
        pass.set_execute(
            [data, roughness, level](rdg_command& command)
            {
                prefilter_cs::prefilter_data prefilter_constant = {
                    .width = data.extent.width,
                    .height = data.extent.height,
                    .cube_map = data.cube_map->get_handle(),
                    .prefilter_map = data.prefilter_map->get_handle(),
                    .roughness = roughness,
                    .level = level,
                };
                data.parameter->set_constant(0, &prefilter_constant, sizeof(prefilter_constant));

                rdg_compute_pipeline pipeline = {
                    .compute_shader = render_device::instance().get_shader<prefilter_cs>(),
                };

                command.set_pipeline(pipeline);
                command.set_parameter(0, render_device::instance().get_bindless_parameter());
                command.set_parameter(1, data.parameter);
                command.dispatch_3d(data.extent.width, data.extent.height, 6, 8, 8, 1);
            });
    }

    static void add_brdf_pass(render_graph& graph, rhi_texture* brdf_map)
    {
        rdg_texture* render_target = graph.add_texture(
            "BRDF LUT",
            brdf_map,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        auto& brdf_pass = graph.add_pass<rdg_render_pass>("BRDF Pass");
        brdf_pass.add_render_target(render_target, RHI_ATTACHMENT_LOAD_OP_DONT_CARE);
        brdf_pass.set_execute(
            [brdf_map](rdg_command& command)
            {
                rhi_texture_extent extent = brdf_map->get_extent();

                rhi_viewport viewport = {
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = static_cast<float>(extent.width),
                    .height = static_cast<float>(extent.height),
                    .min_depth = 0.0f,
                    .max_depth = 1.0f};
                rhi_scissor_rect scissor =
                    {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};

                rdg_render_pipeline pipeline = {};
                pipeline.vertex_shader = render_device::instance().get_shader<fullscreen_vs>();
                pipeline.fragment_shader = render_device::instance().get_shader<brdf_lut_fs>();
                command.set_viewport(viewport);
                command.set_scissor(std::span(&scissor, 1));
                command.set_pipeline(pipeline);
                command.draw_fullscreen();
            });
    }
};

void ibl_tool::generate_cube_map(rhi_texture* env_map, rhi_texture* cube_map)
{
    rdg_allocator allocator;
    render_graph graph(&allocator);

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
    render_graph graph(&allocator);

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
    render_graph graph(&allocator);

    ibl_renderer::render_brdf_lut(graph, brdf_lut);

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    graph.compile();
    graph.record(command);

    device.execute_sync(command);
}
} // namespace violet