#include "graphics/renderers/passes/shading_pass.hpp"
#include "graphics/material_manager.hpp"
#include <algorithm>

namespace violet
{
struct shading_init_worklist_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/shading/init_worklist.hlsl";

    struct constant_data
    {
        std::uint32_t shading_dispatch_buffer;
        std::uint32_t shading_model_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct shading_build_worklist_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/shading/build_worklist.hlsl";

    struct constant_data
    {
        std::uint32_t gbuffer_normal;
        std::uint32_t worklist_buffer;
        std::uint32_t shading_dispatch_buffer;
        std::uint32_t tile_count;
        std::uint32_t width;
        std::uint32_t height;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct shadow_mask_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/shading/shadow_mask.hlsl";

    struct constant_data
    {
        std::uint32_t depth_buffer;
        std::uint32_t normal_buffer;
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_shadow_map;
        std::uint32_t vsm_directional_buffer;
        std::uint32_t light_id;
        float normal_offset;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

void shading_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Shading Pass");

    m_shadow_mask = graph.add_texture(
        "Shadow Mask",
        parameter.render_target->get_extent(),
        RHI_FORMAT_R8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    add_clear_pass(graph, parameter);
    add_tile_classify_pass(graph, parameter);

    const auto& scene = graph.get_scene();

    // Shadow casting lights.
    std::uint32_t shadow_casting_light_count = scene.get_light_count(true);
    for (std::uint32_t light_id = 0; light_id < shadow_casting_light_count; ++light_id)
    {
        add_shadow_mask_pass(graph, parameter, light_id);
        add_tile_shading_pass(graph, parameter, LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED, light_id);
    }

    // Non-shadow casting lights.
    add_tile_shading_pass(graph, parameter, LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED);

    // Indirect lighting.
    add_tile_shading_pass(graph, parameter, LIGHTING_STAGE_INDIRECT_LIGHTING);
}

void shading_pass::add_clear_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_ref render_target;
    };

    graph.add_pass<pass_data>(
        "Clear Pass",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target = pass.add_texture(
                parameter.render_target,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE,
                RHI_TEXTURE_LAYOUT_TRANSFER_DST);
        },
        [](const pass_data& data, rdg_command& command)
        {
            rhi_clear_value value = {};

            std::array<rhi_texture_region, 1> regions;
            regions[0] = {
                .extent = data.render_target.get_extent(),
                .layer_count = 1,
                .aspect = RHI_TEXTURE_ASPECT_COLOR,
            };

            command.clear_texture(data.render_target.get_rhi(), value, regions);
        });
}

void shading_pass::add_tile_classify_pass(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Tile Classify");

    auto extent = parameter.gbuffers[0]->get_extent();
    m_tile_count = ((extent.width + tile_size - 1) / tile_size) *
                   ((extent.height + tile_size - 1) / tile_size);

    m_shading_model_count =
        render_device::instance().get_material_manager()->get_max_shading_model_id() + 1;

    std::uint32_t worklist_size = math::next_power_of_two(m_tile_count * m_shading_model_count);

    m_worklist_buffer = graph.add_buffer(
        "Worklist Buffer",
        worklist_size * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE);

    m_shading_dispatch_buffer = graph.add_buffer(
        "Shading Dispatch Buffer",
        m_shading_model_count * sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    struct init_data
    {
        rdg_buffer_uav shading_dispatch_buffer;
    };

    graph.add_pass<init_data>(
        "Init Worklist",
        RDG_PASS_COMPUTE,
        [&](init_data& data, rdg_pass& pass)
        {
            data.shading_dispatch_buffer =
                pass.add_buffer_uav(m_shading_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [shading_model_count = m_shading_model_count](const init_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<shading_init_worklist_cs>(),
            });
            command.set_constant(
                shading_init_worklist_cs::constant_data{
                    .shading_dispatch_buffer = data.shading_dispatch_buffer.get_bindless(),
                    .shading_model_count = shading_model_count,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(shading_model_count, 32);
        });

    struct build_data
    {
        rdg_texture_srv gbuffer_normal;
        rdg_buffer_uav worklist_buffer;
        rdg_buffer_uav shading_dispatch_buffer;
    };

    graph.add_pass<build_data>(
        "Build Worklist",
        RDG_PASS_COMPUTE,
        [&](build_data& data, rdg_pass& pass)
        {
            data.gbuffer_normal = pass.add_texture_srv(
                parameter.gbuffers[SHADING_GBUFFER_NORMAL],
                RHI_PIPELINE_STAGE_COMPUTE);
            data.worklist_buffer =
                pass.add_buffer_uav(m_worklist_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.shading_dispatch_buffer =
                pass.add_buffer_uav(m_shading_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [tile_count = m_tile_count](const build_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            auto extent = data.gbuffer_normal.get_extent();

            command.set_pipeline({
                .compute_shader = device.get_shader<shading_build_worklist_cs>(),
            });
            command.set_constant(
                shading_build_worklist_cs::constant_data{
                    .gbuffer_normal = data.gbuffer_normal.get_bindless(),
                    .worklist_buffer = data.worklist_buffer.get_bindless(),
                    .shading_dispatch_buffer = data.shading_dispatch_buffer.get_bindless(),
                    .tile_count = tile_count,
                    .width = extent.width,
                    .height = extent.height,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(extent.width, extent.height, tile_size, tile_size);
        });
}

void shading_pass::add_tile_shading_pass(
    render_graph& graph,
    const parameter& parameter,
    lighting_stage stage,
    std::uint32_t light_id) const
{
    rdg_scope scope(graph, "Tile Shading");

    struct pass_data
    {
        std::vector<rdg_texture_srv> gbuffers;
        std::vector<rdg_texture_srv> auxiliary_buffers;
        rdg_texture_uav render_target;
        rdg_buffer_srv worklist_buffer;
        rdg_buffer_ref shading_dispatch_buffer;
        rdg_texture_srv shadow_mask;
        std::uint32_t light_id;

        lighting_stage stage;
    };

    graph.get_scene().each_shading_model(
        [&](std::uint32_t shading_model_id, shading_model_base* shading_model)
        {
            graph.add_pass<pass_data>(
                shading_model->get_name(),
                RDG_PASS_COMPUTE,
                [&](pass_data& data, rdg_pass& pass)
                {
                    data.gbuffers.resize(parameter.gbuffers.size());
                    std::ranges::fill(data.gbuffers, rdg_texture_srv{});

                    for (auto gbuffer : shading_model->get_required_gbuffers())
                    {
                        data.gbuffers[gbuffer] = pass.add_texture_srv(
                            parameter.gbuffers[gbuffer],
                            RHI_PIPELINE_STAGE_COMPUTE);
                    }

                    data.auxiliary_buffers.resize(parameter.auxiliary_buffers.size());
                    std::ranges::fill(data.auxiliary_buffers, rdg_texture_srv{});

                    for (auto auxiliary_buffer : shading_model->get_required_auxiliary_buffers())
                    {
                        if (parameter.auxiliary_buffers[auxiliary_buffer] != nullptr)
                        {
                            data.auxiliary_buffers[auxiliary_buffer] = pass.add_texture_srv(
                                parameter.auxiliary_buffers[auxiliary_buffer],
                                RHI_PIPELINE_STAGE_COMPUTE);
                        }
                    }

                    data.render_target =
                        pass.add_texture_uav(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
                    data.worklist_buffer =
                        pass.add_buffer_srv(m_worklist_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                    data.shading_dispatch_buffer = pass.add_buffer(
                        m_shading_dispatch_buffer,
                        RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                        RHI_ACCESS_INDIRECT_COMMAND_READ);

                    if (stage == LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED)
                    {
                        data.shadow_mask =
                            pass.add_texture_srv(m_shadow_mask, RHI_PIPELINE_STAGE_COMPUTE);
                        data.light_id = light_id;
                    }

                    data.stage = stage;
                },
                [shading_model,
                 shading_model_id,
                 tile_count = m_tile_count](const pass_data& data, rdg_command& command)
                {
                    shading_model->bind(data.auxiliary_buffers, command);

                    auto extent = data.render_target.get_extent();

                    shading_model_cs::constant_data constant = {
                        .render_target = data.render_target.get_bindless(),
                        .width = extent.width,
                        .height = extent.height,
                        .shading_model = shading_model_id,
                        .worklist_buffer = data.worklist_buffer.get_bindless(),
                        .worklist_offset = shading_model_id * tile_count,
                        .stage = static_cast<std::uint32_t>(data.stage),
                    };

                    if (data.stage == LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED)
                    {
                        constant.light_id = data.light_id;
                        constant.shadow_mask = data.shadow_mask.get_bindless();
                    }

                    for (std::uint32_t gbuffer : shading_model->get_required_gbuffers())
                    {
                        constant.gbuffers[gbuffer] = data.gbuffers[gbuffer].get_bindless();
                    }

                    const auto& required_auxiliary_buffers =
                        shading_model->get_required_auxiliary_buffers();
                    for (std::uint32_t i = 0; i < required_auxiliary_buffers.size(); ++i)
                    {
                        auto srv = data.auxiliary_buffers[required_auxiliary_buffers[i]];
                        constant.auxiliary_buffers[i] = srv ? srv.get_bindless() : 0;
                    }

                    command.set_constant(constant);
                    command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                    command.set_parameter(1, RDG_PARAMETER_SCENE);
                    command.set_parameter(2, RDG_PARAMETER_CAMERA);

                    command.dispatch_indirect(
                        data.shading_dispatch_buffer.get_rhi(),
                        shading_model_id * sizeof(shader::dispatch_command));
                });
        });
}

void shading_pass::add_shadow_mask_pass(
    render_graph& graph,
    const parameter& parameter,
    std::uint32_t light_id)
{
    struct pass_data
    {
        rdg_texture_srv depth_buffer;
        rdg_texture_srv normal_buffer;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_texture_srv vsm_physical_shadow_map;
        rdg_buffer_srv vsm_directional_buffer;
        std::uint32_t light_id;

        float normal_offset;
    };

    graph.add_pass<pass_data>(
        "Shadow Mask Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.depth_buffer = pass.add_texture_srv(
                parameter.auxiliary_buffers[SHADING_AUXILIARY_BUFFER_DEPTH],
                RHI_PIPELINE_STAGE_FRAGMENT);
            data.normal_buffer = pass.add_texture_srv(
                parameter.gbuffers[SHADING_GBUFFER_NORMAL],
                RHI_PIPELINE_STAGE_FRAGMENT);
            data.vsm_buffer =
                pass.add_buffer_srv(parameter.vsm_buffer, RHI_PIPELINE_STAGE_FRAGMENT);
            data.vsm_virtual_page_table =
                pass.add_buffer_srv(parameter.vsm_virtual_page_table, RHI_PIPELINE_STAGE_FRAGMENT);
            data.vsm_physical_shadow_map = pass.add_texture_srv(
                parameter.vsm_physical_shadow_map,
                RHI_PIPELINE_STAGE_FRAGMENT);
            data.vsm_directional_buffer =
                pass.add_buffer_srv(parameter.vsm_directional_buffer, RHI_PIPELINE_STAGE_FRAGMENT);

            pass.add_render_target(m_shadow_mask, RHI_ATTACHMENT_LOAD_OP_CLEAR);

            data.light_id = light_id;
            data.normal_offset = parameter.shadow_normal_offset;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .vertex_shader = device.get_shader<fullscreen_vs>(),
                .fragment_shader = device.get_shader<shadow_mask_fs>(),
            });

            command.set_viewport();
            command.set_scissor();

            command.set_constant(
                shadow_mask_fs::constant_data{
                    .depth_buffer = data.depth_buffer.get_bindless(),
                    .normal_buffer = data.normal_buffer.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_physical_shadow_map = data.vsm_physical_shadow_map.get_bindless(),
                    .vsm_directional_buffer = data.vsm_directional_buffer.get_bindless(),
                    .light_id = data.light_id,
                    .normal_offset = data.normal_offset,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.draw_fullscreen();
        });
}
} // namespace violet