#include "graphics/tools/ibl_tool.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
struct cube_sample_vs : public vertex_shader<cube_sample_vs>
{
    static constexpr std::string_view path = "engine/shaders/cube_sample.vs";
};

struct convert_fs : public fragment_shader<convert_fs>
{
    static constexpr std::string_view path = "engine/shaders/equirectangular_to_cubemap.fs";

    static constexpr parameter convert = {
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1}
    };

    static constexpr parameter_slot parameters[] = {
        {0, convert}
    };
};

struct irradiance_fs : public fragment_shader<irradiance_fs>
{
    static constexpr std::string_view path = "engine/shaders/irradiance.fs";

    static constexpr parameter irradiance = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(rhi_texture_extent)},
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1                         }
    };

    static constexpr parameter_slot parameters[] = {
        {0, irradiance}
    };
};

struct prefilter_fs : public fragment_shader<prefilter_fs>
{
    static constexpr std::string_view path = "engine/shaders/prefilter.fs";

    static constexpr parameter prefilter = {
        {RHI_PARAMETER_UNIFORM, RHI_SHADER_STAGE_FRAGMENT, sizeof(float)}, // roughness
        {RHI_PARAMETER_TEXTURE, RHI_SHADER_STAGE_FRAGMENT, 1            }
    };

    static constexpr parameter_slot parameters[] = {
        {0, prefilter}
    };
};

struct brdf_lut_fs : public fragment_shader<brdf_lut_fs>
{
    static constexpr std::string_view path = "engine/shaders/brdf_lut.fs";
};

class ibl_renderer
{
public:
    void render(
        render_graph& graph,
        rhi_texture* env_map,
        rhi_texture* cube_map,
        rhi_texture* irradiance_map,
        rhi_texture* prefilter_map,
        rhi_texture* brdf_map)
    {
        if (env_map != nullptr)
            add_convert_pass(graph, env_map, cube_map);

        add_irradiance_pass(graph, cube_map, irradiance_map);
        add_prefilter_pass(graph, cube_map, prefilter_map);
        add_brdf_pass(graph, brdf_map);
    }

private:
    void add_convert_pass(render_graph& graph, rhi_texture* env_map, rhi_texture* cube_map)
    {
        struct pass_data : public rdg_data
        {
            rhi_viewport viewport;
            rhi_scissor_rect scissor;
            rhi_parameter* parameter;
        };

        rhi_texture_extent extent = cube_map->get_extent();

        pass_data& data = graph.allocate_data<pass_data>();
        data.viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .min_depth = 0.0f,
            .max_depth = 1.0f};
        data.scissor = {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};
        data.parameter = graph.allocate_parameter(convert_fs::convert);
        data.parameter->set_texture(0, env_map, graph.allocate_sampler({}));

        rdg_render_pass* convert_pass = graph.add_pass<rdg_render_pass>("Convert Pass");
        for (std::size_t i = 0; i < 6; ++i)
        {
            rdg_texture* render_target = graph.add_texture(
                "Cube Map " + m_sides[i],
                cube_map,
                0,
                i,
                RHI_TEXTURE_LAYOUT_UNDEFINED,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
            convert_pass->add_render_target(render_target, RHI_ATTACHMENT_LOAD_OP_DONT_CARE);
        }
        convert_pass->set_execute(
            [&data](rdg_command* command)
            {
                rdg_render_pipeline pipeline = {};
                pipeline.vertex_shader = cube_sample_vs::get_rhi();
                pipeline.fragment_shader = convert_fs::get_rhi();

                command->set_viewport(data.viewport);
                command->set_scissor(std::span(&data.scissor, 1));
                command->set_pipeline(pipeline);
                command->set_parameter(0, data.parameter);
                command->draw(0, 6);
            });
    }

    void add_irradiance_pass(
        render_graph& graph,
        rhi_texture* cube_map,
        rhi_texture* irradiance_map)
    {
        struct pass_data : public rdg_data
        {
            rhi_viewport viewport;
            rhi_scissor_rect scissor;
            rhi_parameter* parameter;
        };

        rhi_texture_extent extent = irradiance_map->get_extent();

        pass_data& data = graph.allocate_data<pass_data>();
        data.viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .min_depth = 0.0f,
            .max_depth = 1.0f};
        data.scissor = {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};
        data.parameter = graph.allocate_parameter(irradiance_fs::irradiance);
        data.parameter->set_uniform(0, &extent, sizeof(rhi_texture_extent), 0);
        data.parameter->set_texture(1, cube_map, graph.allocate_sampler({}));

        rdg_render_pass* irradiance_pass = graph.add_pass<rdg_render_pass>("Irradiance Pass");
        for (std::size_t i = 0; i < 6; ++i)
        {
            rdg_texture* render_target = graph.add_texture(
                "Irradiance Map " + m_sides[i],
                irradiance_map,
                0,
                i,
                RHI_TEXTURE_LAYOUT_UNDEFINED,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
            irradiance_pass->add_render_target(render_target, RHI_ATTACHMENT_LOAD_OP_DONT_CARE);
        }
        irradiance_pass->set_execute(
            [&data](rdg_command* command)
            {
                rdg_render_pipeline pipeline = {};
                pipeline.vertex_shader = cube_sample_vs::get_rhi();
                pipeline.fragment_shader = irradiance_fs::get_rhi();

                command->set_viewport(data.viewport);
                command->set_scissor(std::span(&data.scissor, 1));
                command->set_pipeline(pipeline);
                command->set_parameter(0, data.parameter);
                command->draw(0, 6);
            });
    }

    void add_prefilter_pass(render_graph& graph, rhi_texture* cube_map, rhi_texture* prefilter_map)
    {
        rhi_texture_extent extent = prefilter_map->get_extent();

        std::uint32_t level_count = prefilter_map->get_level_count();
        for (std::uint32_t level = 0; level < level_count; ++level)
        {
            struct pass_data : public rdg_data
            {
                rhi_viewport viewport;
                rhi_scissor_rect scissor;
                rhi_parameter* parameter;
            };

            std::uint32_t width = std::max(1u, extent.width >> level);
            std::uint32_t height = std::max(1u, extent.height >> level);
            float roughness = static_cast<float>(level) / static_cast<float>(level_count);

            pass_data& data = graph.allocate_data<pass_data>();
            data.viewport = {
                .x = 0.0f,
                .y = 0.0f,
                .width = static_cast<float>(width),
                .height = static_cast<float>(height),
                .min_depth = 0.0f,
                .max_depth = 1.0f};
            data.scissor = {.min_x = 0, .min_y = 0, .max_x = width, .max_y = height};
            data.parameter = graph.allocate_parameter(prefilter_fs::prefilter);
            data.parameter->set_uniform(0, &roughness, sizeof(float), 0);
            data.parameter->set_texture(1, cube_map, graph.allocate_sampler({}));

            rdg_render_pass* prefilter_pass = graph.add_pass<rdg_render_pass>("Prefilter Pass");
            for (std::size_t i = 0; i < 6; ++i)
            {
                rdg_texture* render_target = graph.add_texture(
                    "Prefilter Map " + m_sides[i] + " Mip " + std::to_string(level),
                    prefilter_map,
                    level,
                    i,
                    RHI_TEXTURE_LAYOUT_UNDEFINED,
                    RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
                prefilter_pass->add_render_target(render_target, RHI_ATTACHMENT_LOAD_OP_DONT_CARE);
            }
            prefilter_pass->set_execute(
                [&data](rdg_command* command)
                {
                    rdg_render_pipeline pipeline = {};
                    pipeline.vertex_shader = cube_sample_vs::get_rhi();
                    pipeline.fragment_shader = prefilter_fs::get_rhi();

                    command->set_viewport(data.viewport);
                    command->set_scissor(std::span(&data.scissor, 1));
                    command->set_pipeline(pipeline);
                    command->set_parameter(0, data.parameter);
                    command->draw(0, 6);
                });
        }
    }

    void add_brdf_pass(render_graph& graph, rhi_texture* brdf_map)
    {
        rdg_texture* render_target = graph.add_texture(
            "BRDF LUT",
            brdf_map,
            RHI_TEXTURE_LAYOUT_UNDEFINED,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

        rdg_render_pass* brdf_pass = graph.add_pass<rdg_render_pass>("BRDF Pass");
        brdf_pass->add_render_target(render_target, RHI_ATTACHMENT_LOAD_OP_DONT_CARE);
        brdf_pass->set_execute(
            [brdf_map](rdg_command* command)
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
                pipeline.vertex_shader = full_screen_vs::get_rhi();
                pipeline.fragment_shader = brdf_lut_fs::get_rhi();
                command->set_viewport(viewport);
                command->set_scissor(std::span(&scissor, 1));
                command->set_pipeline(pipeline);
                command->draw(0, 6);
            });
    }

    const std::vector<std::string> m_sides = {"Right", "Left", "Up", "Bottom", "Front", "Back"};
};

void ibl_tool::generate(
    rhi_texture* env_map,
    rhi_texture* cube_map,
    rhi_texture* irradiance_map,
    rhi_texture* prefilter_map,
    rhi_texture* brdf_map)
{
    rdg_allocator allocator;
    render_graph graph(&allocator);

    ibl_renderer renderer;
    renderer.render(graph, env_map, cube_map, irradiance_map, prefilter_map, brdf_map);

    rhi_command* command = render_device::instance().allocate_command();
    graph.compile();
    graph.execute(command);
    render_device::instance().execute(
        std::span<rhi_command*>(&command, 1),
        std::span<rhi_semaphore*>(),
        std::span<rhi_semaphore*>(),
        nullptr);
}
} // namespace violet