#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/renderers/features/gtao_render_feature.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"
#include "graphics/renderers/features/vsm_render_feature.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"
#include "graphics/renderers/passes/cull_pass.hpp"
#include "graphics/renderers/passes/gbuffer_pass.hpp"
#include "graphics/renderers/passes/gtao_pass.hpp"
#include "graphics/renderers/passes/hzb_pass.hpp"
#include "graphics/renderers/passes/motion_vector_pass.hpp"
#include "graphics/renderers/passes/shading_pass.hpp"
#include "graphics/renderers/passes/shadow_pass.hpp"
#include "graphics/renderers/passes/skybox_pass.hpp"
#include "graphics/renderers/passes/taa_pass.hpp"
#include "graphics/renderers/passes/tone_mapping_pass.hpp"

namespace violet
{
deferred_renderer::deferred_renderer()
{
    add_feature<vsm_render_feature>();
    add_feature<taa_render_feature>();
    add_feature<gtao_render_feature>();
}

void deferred_renderer::on_render(render_graph& graph)
{
    prepare(graph);

    if (graph.get_scene().get_instance_count() != 0)
    {
        {
            rdg_scope scope(graph, "Main Pass");
            add_cull_pass(graph, true);
            add_gbuffer_pass(graph, true);
            add_hzb_pass(graph);
        }

        {
            rdg_scope scope(graph, "Post Pass");
            add_cull_pass(graph, false);
            add_gbuffer_pass(graph, false);
            add_hzb_pass(graph);
        }

        if (get_feature<gtao_render_feature>(true))
        {
            add_gtao_pass(graph);
        }
        else
        {
            m_ao_buffer = nullptr;
        }

        add_shadow_pass(graph);
        add_shading_pass(graph);
    }

    if (graph.get_scene().has_skybox())
    {
        graph.add_pass<skybox_pass>({
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = graph.get_scene().get_instance_count() == 0,
        });
    }

    if (get_feature<taa_render_feature>(true))
    {
        add_motion_vector_pass(graph);
        add_taa_pass(graph);
    }

    add_tone_mapping_pass(graph);
    add_present_pass(graph);
}

void deferred_renderer::prepare(render_graph& graph)
{
    m_render_extent = graph.get_camera().get_render_target()->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        m_render_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE |
            RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_TRANSFER_DST);

    m_hzb = graph.add_texture(
        "HZB",
        graph.get_camera().get_hzb(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    const auto& scene = graph.get_scene();

    m_vsm_buffer = graph.add_buffer("VSM Buffer", scene.get_vsm_buffer());
    m_vsm_virtual_page_table =
        graph.add_buffer("VSM Page Table", scene.get_vsm_virtual_page_table());
    m_vsm_physical_page_table =
        graph.add_buffer("VSM Physical Page Table", scene.get_vsm_physical_page_table());
    m_vsm_physical_texture = graph.add_texture(
        "VSM Physical Texture",
        scene.get_vsm_physical_texture(),
        RHI_TEXTURE_LAYOUT_GENERAL,
        RHI_TEXTURE_LAYOUT_GENERAL);

    if (m_debug_mode != DEBUG_MODE_NONE)
    {
        m_debug_output = graph.add_texture(
            "Debug Output",
            m_render_extent,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_TEXTURE_STORAGE | RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_TRANSFER_DST);

        struct pass_data
        {
            rdg_texture_ref debug_output;
        };
        graph.add_pass<pass_data>(
            "Clear Debug Output",
            RDG_PASS_TRANSFER,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.debug_output = pass.add_texture(
                    m_debug_output,
                    RHI_PIPELINE_STAGE_TRANSFER,
                    RHI_ACCESS_TRANSFER_WRITE,
                    RHI_TEXTURE_LAYOUT_TRANSFER_DST);
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_region region = {
                    .extent = data.debug_output.get_extent(),
                    .level = 0,
                    .layer = 0,
                    .layer_count = 1,
                };

                rhi_clear_value clear_value = {};
                clear_value.color.float32[3] = 1.0f;
                command.clear_texture(data.debug_output.get_rhi(), clear_value, {&region, 1});
            });
    }
}

void deferred_renderer::add_cull_pass(render_graph& graph, bool main_pass)
{
    if (main_pass)
    {
        m_cluster_queue = graph.add_buffer(
            "Cluster Queue",
            graphics_config::get_max_candidate_cluster_count() * sizeof(vec3u),
            RHI_BUFFER_STORAGE);
        m_cluster_queue_state = graph.add_buffer(
            "Cluster Queue State",
            6 * sizeof(std::uint32_t),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

        m_draw_buffer = graph.add_buffer(
            "Draw Buffer",
            graph.get_scene().get_instance_capacity() * sizeof(shader::draw_command),
            RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
        m_draw_count_buffer = graph.add_buffer(
            "Draw Count Buffer",
            graph.get_scene().get_batch_capacity() * sizeof(std::uint32_t),
            RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT | RHI_BUFFER_TRANSFER_DST);
        m_draw_info_buffer = graph.add_buffer(
            "Draw Info Buffer",
            graph.get_scene().get_instance_capacity() * sizeof(shader::draw_info),
            RHI_BUFFER_STORAGE);

        m_recheck_instances = graph.add_buffer(
            "Recheck Instances",
            math::next_power_of_two(
                (graph.get_scene().get_instance_count() + 31) / 32 * sizeof(std::uint32_t)),
            RHI_BUFFER_STORAGE);
    }

    graph.add_pass<cull_pass>({
        .stage = main_pass ? CULL_STAGE_MAIN_PASS : CULL_STAGE_POST_PASS,
        .hzb = m_hzb,
        .cluster_queue = m_cluster_queue,
        .cluster_queue_state = m_cluster_queue_state,
        .draw_buffer = m_draw_buffer,
        .draw_count_buffer = m_draw_count_buffer,
        .draw_info_buffer = m_draw_info_buffer,
        .recheck_instances = m_recheck_instances,
    });
}

void deferred_renderer::add_gbuffer_pass(render_graph& graph, bool main_pass)
{
    rdg_scope scope(graph, "GBuffer");

    if (main_pass)
    {
        m_gbuffers.resize(4);
        m_gbuffers[SHADING_GBUFFER_ALBEDO] = graph.add_texture(
            "GBuffer Albedo",
            m_render_extent,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
                RHI_TEXTURE_TRANSFER_DST);
        m_gbuffers[SHADING_GBUFFER_MATERIAL] = graph.add_texture(
            "GBuffer Material",
            m_render_extent,
            RHI_FORMAT_R8G8_UNORM,
            RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
                RHI_TEXTURE_TRANSFER_DST);
        m_gbuffers[SHADING_GBUFFER_NORMAL] = graph.add_texture(
            "GBuffer Normal",
            m_render_extent,
            RHI_FORMAT_R32_UINT,
            RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
                RHI_TEXTURE_TRANSFER_DST);
        m_gbuffers[SHADING_GBUFFER_EMISSIVE] = graph.add_texture(
            "GBuffer Emissive",
            m_render_extent,
            RHI_FORMAT_R8G8B8A8_UNORM,
            RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
                RHI_TEXTURE_TRANSFER_DST);

        m_visibility_buffer = graph.add_texture(
            "Visibility Buffer",
            m_render_extent,
            RHI_FORMAT_R32G32_UINT,
            RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

        m_depth_buffer = graph.add_texture(
            "Depth Buffer",
            m_render_extent,
            RHI_FORMAT_D32_FLOAT,
            RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE);
    }

    graph.add_pass<gbuffer_pass>({
        .draw_buffer = m_draw_buffer,
        .draw_count_buffer = m_draw_count_buffer,
        .draw_info_buffer = m_draw_info_buffer,
        .gbuffers = m_gbuffers,
        .visibility_buffer = m_visibility_buffer,
        .depth_buffer = m_depth_buffer,
        .clear = main_pass,
    });
}

void deferred_renderer::add_hzb_pass(render_graph& graph)
{
    graph.add_pass<hzb_pass>({
        .depth_buffer = m_depth_buffer,
        .hzb = m_hzb,
    });
}

void deferred_renderer::add_gtao_pass(render_graph& graph)
{
    auto* gtao = get_feature<gtao_render_feature>();

    m_ao_buffer = graph.add_texture(
        "AO Buffer",
        m_render_extent,
        RHI_FORMAT_R8_UNORM,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    graph.add_pass<gtao_pass>({
        .slice_count = gtao->get_slice_count(),
        .step_count = gtao->get_step_count(),
        .radius = gtao->get_radius(),
        .falloff = gtao->get_falloff(),
        .hzb = m_hzb,
        .depth_buffer = m_depth_buffer,
        .normal_buffer = m_gbuffers[SHADING_GBUFFER_NORMAL],
        .ao_buffer = m_ao_buffer,
    });
}

void deferred_renderer::add_shadow_pass(render_graph& graph)
{
    auto* vsm = get_feature<vsm_render_feature>();

    shadow_pass::debug_mode debug_mode = shadow_pass::DEBUG_MODE_NONE;
    switch (m_debug_mode)
    {
    case DEBUG_MODE_VSM_PAGE:
        debug_mode = shadow_pass::DEBUG_MODE_PAGE;
        break;
    case DEBUG_MODE_VSM_PAGE_CACHE:
        debug_mode = shadow_pass::DEBUG_MODE_PAGE_CACHE;
        break;
    case DEBUG_MODE_VSM_PHYSICAL_PAGE_TABLE:
        debug_mode = shadow_pass::DEBUG_MODE_PHYSICAL_PAGE_TABLE;
    default:
        break;
    }

    graph.add_pass<shadow_pass>({
        .depth_buffer = m_depth_buffer,
        .vsm_buffer = m_vsm_buffer,
        .vsm_virtual_page_table = m_vsm_virtual_page_table,
        .vsm_physical_page_table = m_vsm_physical_page_table,
        .vsm_physical_texture = m_vsm_physical_texture,
        .lru_state = graph.add_buffer("VSM LRU State", vsm->get_lru_state()),
        .lru_buffer = graph.add_buffer("VSM LRU Buffer", vsm->get_lru_buffer()),
        .lru_curr_index = vsm->get_curr_lru_index(),
        .lru_prev_index = vsm->get_prev_lru_index(),
        .debug_mode = debug_mode,
        .debug_output = m_debug_output,
    });
}

void deferred_renderer::add_shading_pass(render_graph& graph)
{
    std::vector<rdg_texture*> auxiliary_buffers = {
        m_depth_buffer,
        m_ao_buffer,
    };

    graph.add_pass<shading_pass>({
        .gbuffers = m_gbuffers,
        .auxiliary_buffers = auxiliary_buffers,
        .render_target = m_render_target,
        .vsm_buffer = m_vsm_buffer,
        .vsm_virtual_page_table = m_vsm_virtual_page_table,
        .vsm_physical_texture = m_vsm_physical_texture,
    });
}

void deferred_renderer::add_motion_vector_pass(render_graph& graph)
{
    m_motion_vectors = graph.add_texture(
        "Motion Vectors",
        m_render_extent,
        RHI_FORMAT_R16G16_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    graph.add_pass<motion_vector_pass>({
        .depth_buffer = m_depth_buffer,
        .motion_vector = m_motion_vectors,
    });
}

void deferred_renderer::add_taa_pass(render_graph& graph)
{
    auto* taa = get_feature<taa_render_feature>();

    rdg_texture* resolved_render_target = graph.add_texture(
        "Resolved Render Target",
        taa->get_current(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    rdg_texture* history_render_target = nullptr;
    if (taa->is_history_valid())
    {
        history_render_target = graph.add_texture(
            "History Render Target",
            taa->get_history(),
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    }

    graph.add_pass<taa_pass>({
        .current_render_target = m_render_target,
        .history_render_target = history_render_target,
        .depth_buffer = m_depth_buffer,
        .motion_vector = m_motion_vectors,
        .resolved_render_target = resolved_render_target,
    });

    m_render_target = resolved_render_target;
}

void deferred_renderer::add_tone_mapping_pass(render_graph& graph)
{
    rdg_texture* ldr_target = graph.add_texture(
        "LDR Target",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_STORAGE);

    graph.add_pass<tone_mapping_pass>({
        .hdr_texture = m_render_target,
        .ldr_texture = ldr_target,
    });

    m_render_target = ldr_target;
}

void deferred_renderer::add_present_pass(render_graph& graph)
{
    rdg_texture* camera_output = graph.add_texture(
        "Camera Output",
        graph.get_camera().get_render_target(),
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_PRESENT);

    rhi_texture_region region = {
        .offset_x = 0,
        .offset_y = 0,
        .extent = m_render_extent,
        .level = 0,
        .layer = 0,
        .layer_count = 1,
    };

    graph.add_pass<blit_pass>({
        .src = m_debug_mode == DEBUG_MODE_NONE ? m_render_target : m_debug_output,
        .src_region = region,
        .dst = camera_output,
        .dst_region = region,
    });

    m_render_target = camera_output;
}
} // namespace violet