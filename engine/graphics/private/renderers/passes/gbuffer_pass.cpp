#include "graphics/renderers/passes/gbuffer_pass.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/renderers/passes/mesh_pass.hpp"
#include "graphics/renderers/passes/scan_pass.hpp"
#include <format>

namespace violet
{
struct visibility_init_worklist_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/visibility/init_worklist.hlsl";

    struct constant_data
    {
        std::uint32_t worklist_size_buffer;
        std::uint32_t sort_dispatch_buffer;
        std::uint32_t resolve_dispatch_buffer;
        std::uint32_t material_offset_buffer;
        std::uint32_t material_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct visibility_build_worklist_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/visibility/build_worklist.hlsl";

    struct constant_data
    {
        std::uint32_t visibility_buffer;
        std::uint32_t worklist_buffer;
        std::uint32_t worklist_size_buffer;
        std::uint32_t material_offset_buffer;
        std::uint32_t sort_dispatch_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

struct visibility_sort_worklist_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/visibility/sort_worklist.hlsl";

    struct constant_data
    {
        std::uint32_t raw_worklist_buffer;
        std::uint32_t worklist_buffer;
        std::uint32_t worklist_size_buffer;
        std::uint32_t material_offset_buffer;
        std::uint32_t resolve_dispatch_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void gbuffer_pass::add(render_graph& graph, const parameter& parameter)
{
    if (parameter.clear)
    {
        add_clear_pass(graph, parameter);
    }

    add_visibility_pass(graph, parameter);
    add_deferred_pass(graph, parameter);
}

void gbuffer_pass::add_clear_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        std::vector<rdg_texture_ref> gbuffers;
    };

    graph.add_pass<pass_data>(
        "Clear Pass",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.gbuffers.clear();
            for (auto* gbuffer : parameter.gbuffers)
            {
                data.gbuffers.push_back(pass.add_texture(
                    gbuffer,
                    RHI_PIPELINE_STAGE_TRANSFER,
                    RHI_ACCESS_TRANSFER_WRITE,
                    RHI_TEXTURE_LAYOUT_TRANSFER_DST));
            }
        },
        [](const pass_data& data, rdg_command& command)
        {
            rhi_clear_value value = {};

            std::array<rhi_texture_region, 1> regions;
            regions[0] = {
                .extent = data.gbuffers[0].get_extent(),
                .layer_count = 1,
                .aspect = RHI_TEXTURE_ASPECT_COLOR,
            };

            for (const auto& gbuffer : data.gbuffers)
            {
                command.clear_texture(gbuffer.get_rhi(), value, regions);
            }
        });
}

void gbuffer_pass::add_visibility_pass(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Visibility Pass");

    auto extent = parameter.gbuffers[0]->get_extent();
    m_tile_count = ((extent.width + tile_size - 1) / tile_size) *
                   ((extent.height + tile_size - 1) / tile_size);

    m_material_count =
        render_device::instance().get_material_manager()->get_max_material_resolve_pipeline_id() +
        1;

    m_visibility_buffer = parameter.visibility_buffer;

    rhi_clear_value clear_value = {};
    clear_value.color.uint32[0] = 0xFFFFFFFF;

    graph.add_pass<mesh_pass>({
        .draw_buffer = parameter.draw_buffer,
        .draw_count_buffer = parameter.draw_count_buffer,
        .draw_info_buffer = parameter.draw_info_buffer,
        .render_targets = {&m_visibility_buffer, 1},
        .depth_buffer = parameter.depth_buffer,
        .surface_type = SURFACE_TYPE_OPAQUE,
        .material_path = MATERIAL_PATH_VISIBILITY,
        .clear = parameter.clear,
        .render_target_clear_values = {&clear_value, 1},
    });

    add_material_classify_pass(graph);

    graph.get_scene().each_material_resolve_pipeline(
        [&](std::uint32_t pipeline_id, const rdg_compute_pipeline& pipeline)
        {
            add_material_resolve_pass(graph, parameter, pipeline_id, pipeline);
        });
}

void gbuffer_pass::add_material_classify_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Material Classify");

    std::uint32_t worklist_size =
        math::next_power_of_two(m_tile_count * std::min(m_material_count, tile_size * tile_size));

    rdg_buffer* raw_worklist_buffer =
        graph.add_buffer("Worklist Buffer", worklist_size * sizeof(vec2u), RHI_BUFFER_STORAGE);

    m_worklist_size_buffer =
        graph.add_buffer("Worklist Size Buffer", sizeof(std::uint32_t), RHI_BUFFER_STORAGE);

    m_material_offset_buffer = graph.add_buffer(
        "Material Offset Buffer",
        m_material_count * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE);

    rdg_buffer* sort_dispatch_buffer = graph.add_buffer(
        "Sort Worklist Dispatch Buffer",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_resolve_dispatch_buffer = graph.add_buffer(
        "Material Resolve Dispatch Buffer",
        math::next_power_of_two(m_tile_count) * sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    struct init_data
    {
        rdg_buffer_uav worklist_size_buffer;
        rdg_buffer_uav sort_dispatch_buffer;
        rdg_buffer_uav resolve_dispatch_buffer;
        rdg_buffer_uav material_offset_buffer;
    };

    graph.add_pass<init_data>(
        "Init Worklist",
        RDG_PASS_COMPUTE,
        [&](init_data& data, rdg_pass& pass)
        {
            data.worklist_size_buffer =
                pass.add_buffer_uav(m_worklist_size_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.sort_dispatch_buffer =
                pass.add_buffer_uav(sort_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.resolve_dispatch_buffer =
                pass.add_buffer_uav(m_resolve_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.material_offset_buffer =
                pass.add_buffer_uav(m_material_offset_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [material_count = m_material_count](const init_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<visibility_init_worklist_cs>(),
            });
            command.set_constant(
                visibility_init_worklist_cs::constant_data{
                    .worklist_size_buffer = data.worklist_size_buffer.get_bindless(),
                    .sort_dispatch_buffer = data.sort_dispatch_buffer.get_bindless(),
                    .resolve_dispatch_buffer = data.resolve_dispatch_buffer.get_bindless(),
                    .material_offset_buffer = data.material_offset_buffer.get_bindless(),
                    .material_count = material_count,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.dispatch_1d(material_count);
        });

    struct build_data
    {
        rdg_texture_srv visibility_buffer;
        rdg_buffer_uav worklist_buffer;
        rdg_buffer_uav worklist_size_buffer;
        rdg_buffer_uav material_offset_buffer;
        rdg_buffer_uav sort_dispatch_buffer;
    };

    graph.add_pass<build_data>(
        "Build Worklist",
        RDG_PASS_COMPUTE,
        [&](build_data& data, rdg_pass& pass)
        {
            data.visibility_buffer =
                pass.add_texture_srv(m_visibility_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.worklist_buffer =
                pass.add_buffer_uav(raw_worklist_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.worklist_size_buffer =
                pass.add_buffer_uav(m_worklist_size_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.material_offset_buffer =
                pass.add_buffer_uav(m_material_offset_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.sort_dispatch_buffer =
                pass.add_buffer_uav(sort_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const build_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<visibility_build_worklist_cs>(),
            });
            command.set_constant(
                visibility_build_worklist_cs::constant_data{
                    .visibility_buffer = data.visibility_buffer.get_bindless(),
                    .worklist_buffer = data.worklist_buffer.get_bindless(),
                    .worklist_size_buffer = data.worklist_size_buffer.get_bindless(),
                    .material_offset_buffer = data.material_offset_buffer.get_bindless(),
                    .sort_dispatch_buffer = data.sort_dispatch_buffer.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);

            auto extent = data.visibility_buffer.get_extent();
            command.dispatch_2d(extent.width, extent.height, tile_size, tile_size);
        });

    graph.add_pass<scan_pass>({
        .buffer = m_material_offset_buffer,
        .size = m_material_count,
    });

    m_worklist_buffer = graph.add_buffer(
        "Worklist Buffer",
        worklist_size * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE);

    struct sort_data
    {
        rdg_buffer_srv raw_worklist_buffer;
        rdg_buffer_uav worklist_buffer;
        rdg_buffer_srv worklist_size_buffer;
        rdg_buffer_srv material_offset_buffer;
        rdg_buffer_uav resolve_dispatch_buffer;
        rdg_buffer_ref sort_dispatch_buffer;
    };

    graph.add_pass<sort_data>(
        "Sort Worklist",
        RDG_PASS_COMPUTE,
        [&](sort_data& data, rdg_pass& pass)
        {
            data.raw_worklist_buffer =
                pass.add_buffer_srv(raw_worklist_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.worklist_buffer =
                pass.add_buffer_uav(m_worklist_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.worklist_size_buffer =
                pass.add_buffer_srv(m_worklist_size_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.material_offset_buffer =
                pass.add_buffer_srv(m_material_offset_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.resolve_dispatch_buffer =
                pass.add_buffer_uav(m_resolve_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.sort_dispatch_buffer = pass.add_buffer(
                sort_dispatch_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const sort_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();
            command.set_pipeline({
                .compute_shader = device.get_shader<visibility_sort_worklist_cs>(),
            });
            command.set_constant(
                visibility_sort_worklist_cs::constant_data{
                    .raw_worklist_buffer = data.raw_worklist_buffer.get_bindless(),
                    .worklist_buffer = data.worklist_buffer.get_bindless(),
                    .worklist_size_buffer = data.worklist_size_buffer.get_bindless(),
                    .material_offset_buffer = data.material_offset_buffer.get_bindless(),
                    .resolve_dispatch_buffer = data.resolve_dispatch_buffer.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.dispatch_indirect(data.sort_dispatch_buffer.get_rhi(), 0);
        });
}

void gbuffer_pass::add_material_resolve_pass(
    render_graph& graph,
    const parameter& parameter,
    std::uint32_t pipeline_id,
    const rdg_compute_pipeline& pipeline)
{
    struct pass_data
    {
        std::vector<rdg_texture_uav> gbuffers;

        rdg_texture_srv visibility_buffer;
        rdg_buffer_srv worklist_buffer;
        rdg_buffer_srv material_offset_buffer;

        rdg_buffer_ref dispatch_buffer;
        rdg_compute_pipeline pipeline;
    };

    graph.add_pass<pass_data>(
        std::format("Material Resolve: {}", pipeline_id),
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visibility_buffer =
                pass.add_texture_srv(m_visibility_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.worklist_buffer =
                pass.add_buffer_srv(m_worklist_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.material_offset_buffer =
                pass.add_buffer_srv(m_material_offset_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.gbuffers.clear();
            for (auto* gbuffer : parameter.gbuffers)
            {
                data.gbuffers.push_back(pass.add_texture_uav(gbuffer, RHI_PIPELINE_STAGE_COMPUTE));
            }

            data.dispatch_buffer = pass.add_buffer(
                m_resolve_dispatch_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);

            data.pipeline = pipeline;
        },
        [pipeline_id](const pass_data& data, rdg_command& command)
        {
            command.set_pipeline(data.pipeline);

            material_resolve_cs::constant_data constant = {
                .visibility_buffer = data.visibility_buffer.get_bindless(),
                .worklist_buffer = data.worklist_buffer.get_bindless(),
                .material_offset_buffer = data.material_offset_buffer.get_bindless(),
                .resolve_pipeline = pipeline_id,
            };

            for (std::uint32_t i = 0; i < data.gbuffers.size(); ++i)
            {
                constant.gbuffers[i] = data.gbuffers[i].get_bindless();
            }

            command.set_constant(constant);
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);
            command.dispatch_indirect(
                data.dispatch_buffer.get_rhi(),
                pipeline_id * sizeof(shader::dispatch_command));
        });
}

void gbuffer_pass::add_deferred_pass(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Deferred Pass");

    graph.add_pass<mesh_pass>({
        .draw_buffer = parameter.draw_buffer,
        .draw_count_buffer = parameter.draw_count_buffer,
        .draw_info_buffer = parameter.draw_info_buffer,
        .render_targets = parameter.gbuffers,
        .depth_buffer = parameter.depth_buffer,
        .surface_type = SURFACE_TYPE_OPAQUE,
        .material_path = MATERIAL_PATH_DEFERRED,
        .clear = false,
    });
}
} // namespace violet