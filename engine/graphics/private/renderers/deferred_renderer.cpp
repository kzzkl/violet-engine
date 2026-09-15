#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/render_scene/render_scene_camera.hpp"
#include "graphics/render_scene/render_scene_environment.hpp"
#include "graphics/render_scene/render_scene_mesh.hpp"
#include "graphics/render_scene/render_scene_shadow.hpp"
#include "graphics/renderers/features/atmosphere_feature.hpp"
#include "graphics/renderers/features/bloom_feature.hpp"
#include "graphics/renderers/features/cluster_feature.hpp"
#include "graphics/renderers/features/dithering_feature.hpp"
#include "graphics/renderers/features/eye_adaptation_feature.hpp"
#include "graphics/renderers/features/gtao_feature.hpp"
#include "graphics/renderers/features/shadow_feature.hpp"
#include "graphics/renderers/features/ssgi_feature.hpp"
#include "graphics/renderers/features/taa_feature.hpp"
#include "graphics/renderers/features/vsm_feature.hpp"
#include "graphics/renderers/gbuffer.hpp"
#include "graphics/renderers/passes/atmosphere_pass.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"
#include "graphics/renderers/passes/bloom_pass.hpp"
#include "graphics/renderers/passes/cull_pass.hpp"
#include "graphics/renderers/passes/dithering_pass.hpp"
#include "graphics/renderers/passes/eye_adaptation_pass.hpp"
#include "graphics/renderers/passes/gbuffer_pass.hpp"
#include "graphics/renderers/passes/gtao_pass.hpp"
#include "graphics/renderers/passes/hzb_pass.hpp"
#include "graphics/renderers/passes/motion_vector_pass.hpp"
#include "graphics/renderers/passes/sdf_pass.hpp"
#include "graphics/renderers/passes/shading_pass.hpp"
#include "graphics/renderers/passes/shadow_pass.hpp"
#include "graphics/renderers/passes/skybox_pass.hpp"
#include "graphics/renderers/passes/ssgi_pass.hpp"
#include "graphics/renderers/passes/taa_pass.hpp"
#include "graphics/renderers/passes/tone_mapping_pass.hpp"

namespace violet
{
namespace
{
std::uint32_t previous_pow2(std::uint32_t v)
{
    std::uint32_t r = 1;

    while (r * 2 < v)
    {
        r *= 2;
    }

    return r;
}
} // namespace

deferred_renderer::deferred_renderer()
{
    add_feature<shadow_feature>();
    add_feature<vsm_feature>();
    add_feature<taa_feature>();
    add_feature<gtao_feature>();
    add_feature<ssgi_feature>();
    add_feature<eye_adaptation_feature>();
    add_feature<bloom_feature>();
    add_feature<atmosphere_feature>();
    add_feature<dithering_feature>();
    add_feature<cluster_feature>();
}

void deferred_renderer::on_render(render_graph& graph)
{
    prepare(graph);

    {
        rdg_scope scope(graph, "Main Pass");
        add_cull_pass(graph, true);
        add_gbuffer_pass(graph, true);
        add_culling_hzb_pass(graph);
    }

    {
        rdg_scope scope(graph, "Post Pass");
        add_cull_pass(graph, false);
        add_gbuffer_pass(graph, false);
        add_culling_hzb_pass(graph);
    }

    add_tracing_hzb_pass(graph);

    if (get_feature<gtao_feature>(true))
    {
        add_gtao_pass(graph);
    }
    else
    {
        m_ao_buffer = nullptr;
    }

    add_shadow_pass(graph);

    add_sky_lut_pass(graph);

    if (get_feature<taa_feature>(true) || get_feature<ssgi_feature>(true))
    {
        add_motion_vector_pass(graph);
    }

    add_sdf_pass(graph);

    if (get_feature<ssgi_feature>(true))
    {
        add_ssgi_pass(graph);
    }
    else
    {
        m_indirect_diffuse = nullptr;
    }

    add_shading_pass(graph);

    add_sky_pass(graph);

    {
        rdg_scope scope(graph, "Post Processing");
        if (get_feature<taa_feature>(true))
        {
            add_taa_pass(graph);
        }
        else if (get_feature<ssgi_feature>(true) && m_prev_scene_color != nullptr)
        {
            rhi_texture_region region = {
                .extent = m_render_target->get_extent(),
                .layer_count = 1,
            };

            graph.add_pass<blit_pass>({
                .src = m_render_target,
                .src_region = region,
                .dst = m_prev_scene_color,
                .dst_region = region,
            });
        }

        add_eye_adaptation_pass(graph);
        add_bloom_pass(graph);
        add_dithering_pass(graph);
        add_tone_mapping_pass(graph);
    }

    add_present_pass(graph);
}

void deferred_renderer::prepare(render_graph& graph)
{
    prepare_rhi_resources(graph);
    prepare_rdg_resources(graph);

    m_render_extent = graph.get_context().get_render_target()->get_extent();
}

void deferred_renderer::prepare_rhi_resources(render_graph& graph)
{
    rhi_extent render_extent = graph.get_context().get_render_target()->get_extent();
    bool render_extent_changed = m_render_extent != render_extent;

    if (render_extent_changed || m_resources.hzb == nullptr)
    {
        rhi_extent hzb_extent = {
            .width = previous_pow2(render_extent.width),
            .height = previous_pow2(render_extent.height),
        };

        if (m_resources.hzb == nullptr || m_resources.hzb->get_extent() != hzb_extent)
        {
            std::uint32_t max_size = std::max(hzb_extent.width, hzb_extent.height);
            std::uint32_t level_count =
                static_cast<std::uint32_t>(std::floor(std::log2(max_size))) + 1;

            m_resources.hzb = render_device::instance().create_texture({
                .extent = hzb_extent,
                .format = RHI_FORMAT_R32_FLOAT,
                .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                .level_count = level_count,
                .layer_count = 1,
                .samples = RHI_SAMPLE_COUNT_1,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });
        }
    }

    if (get_feature<taa_feature>(true) || get_feature<ssgi_feature>(true))
    {
        if (m_resources.prev_scene_color == nullptr ||
            m_resources.prev_scene_color->get_extent() != render_extent)
        {
            m_resources.prev_scene_color = render_device::instance().create_texture({
                .extent = render_extent,
                .format = RHI_FORMAT_R16G16B16A16_FLOAT,
                .flags =
                    RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE | RHI_TEXTURE_TRANSFER_DST,
                .level_count = 1,
                .layer_count = 1,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });
            m_prev_scene_color_valid = false;
        }
        else
        {
            m_prev_scene_color_valid = true;
        }
    }
    else
    {
        m_resources.prev_scene_color = nullptr;
        m_prev_scene_color_valid = false;
    }
}

void deferred_renderer::prepare_rdg_resources(render_graph& graph)
{
    const auto& context = graph.get_context();

    rhi_extent render_extent = context.get_render_target()->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        render_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE |
            RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_TRANSFER_DST);

    m_culling_hzb = graph.add_texture(
        "Culling HZB",
        m_resources.hzb.get(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    if (m_resources.prev_scene_color != nullptr)
    {
        m_prev_scene_color = graph.add_texture(
            "Previous Scene Color",
            m_resources.prev_scene_color.get(),
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    }
    else
    {
        m_prev_scene_color = nullptr;
    }

    m_cluster_queue = graph.add_buffer(
        "Cluster Queue",
        graphics_config::get_max_candidate_cluster_count() * sizeof(vec3u),
        RHI_BUFFER_STORAGE);
    m_cluster_queue_state = graph.add_buffer(
        "Cluster Queue State",
        6 * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

    const auto& mesh_module = context.get_module<render_scene_mesh>();

    m_draw_buffer = graph.add_buffer(
        "Draw Buffer",
        mesh_module.get_draw_call_capacity() * sizeof(shader::draw_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
    m_draw_count_buffer = graph.add_buffer(
        "Draw Count Buffer",
        mesh_module.get_batch_capacity() * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT | RHI_BUFFER_TRANSFER_DST);
    m_draw_info_buffer = graph.add_buffer(
        "Draw Info Buffer",
        mesh_module.get_draw_call_capacity() * sizeof(shader::draw_info),
        RHI_BUFFER_STORAGE);

    m_recheck_instances = graph.add_buffer(
        "Recheck Instances",
        math::next_power_of_two(mesh_module.get_instance_count()) * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE);
    m_recheck_count = graph.add_buffer(
        "Recheck Count",
        sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

    m_gbuffers.resize(4);
    m_gbuffers[GBUFFER_ALBEDO] = graph.add_texture(
        "GBuffer Albedo",
        render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
            RHI_TEXTURE_TRANSFER_DST);
    m_gbuffers[GBUFFER_MATERIAL] = graph.add_texture(
        "GBuffer Material",
        render_extent,
        RHI_FORMAT_R8G8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
            RHI_TEXTURE_TRANSFER_DST);
    m_gbuffers[GBUFFER_NORMAL] = graph.add_texture(
        "GBuffer Normal",
        render_extent,
        RHI_FORMAT_R32_UINT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
            RHI_TEXTURE_TRANSFER_DST);
    m_gbuffers[GBUFFER_EMISSIVE] = graph.add_texture(
        "GBuffer Emissive",
        render_extent,
        RHI_FORMAT_R11G11B10_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE |
            RHI_TEXTURE_TRANSFER_DST);

    m_visibility_buffer = graph.add_texture(
        "Visibility Buffer",
        render_extent,
        RHI_FORMAT_R32G32_UINT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        render_extent,
        RHI_FORMAT_D32_FLOAT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE);

    const auto& shadow_module = context.get_module<render_scene_shadow>();

    m_shadow_light_buffer =
        graph.add_buffer("Shadow Light Buffer", shadow_module.get_shadow_light_buffer());

    m_vsm_buffer = graph.add_buffer("VSM Buffer", shadow_module.get_vsm_buffer());
    m_vsm_virtual_page_table =
        graph.add_buffer("VSM Page Table", shadow_module.get_virtual_page_table());
    m_vsm_physical_page_table =
        graph.add_buffer("VSM Physical Page Table", shadow_module.get_physical_page_table());
    m_vsm_physical_shadow_map_static = graph.add_texture(
        "VSM Physical Shadow Map Static",
        shadow_module.get_physical_shadow_map_static(),
        RHI_TEXTURE_LAYOUT_GENERAL,
        RHI_TEXTURE_LAYOUT_GENERAL);
    m_vsm_physical_shadow_map_final = graph.add_texture(
        "VSM Physical Shadow Map Final",
        shadow_module.get_physical_shadow_map_final(),
        RHI_TEXTURE_LAYOUT_GENERAL,
        RHI_TEXTURE_LAYOUT_GENERAL);
    m_vsm_directional_buffer =
        graph.add_buffer("VSM Directional Buffer", shadow_module.get_clipmap_buffer());

    if (shadow_module.get_hzb() != nullptr)
    {
        m_vsm_hzb = graph.add_texture(
            "VSM HZB",
            shadow_module.get_hzb(),
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    }
    else
    {
        m_vsm_hzb = nullptr;
    }

    if (m_debug_mode != DEBUG_MODE_NONE)
    {
        m_debug_output = graph.add_texture(
            "Debug Output",
            render_extent,
            RHI_FORMAT_R32G32B32A32_FLOAT,
            RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_STORAGE | RHI_TEXTURE_TRANSFER_SRC |
                RHI_TEXTURE_TRANSFER_DST);

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
    auto* cluster = get_feature<cluster_feature>(true);

    graph.add_pass<cull_pass>({
        .stage = main_pass ? CULL_STAGE_MAIN_PASS : CULL_STAGE_POST_PASS,
        .hzb = m_culling_hzb,
        .cluster_queue = m_cluster_queue,
        .cluster_queue_state = m_cluster_queue_state,
        .cluster_threshold = cluster->threshold,
        .draw_buffer = m_draw_buffer,
        .draw_count_buffer = m_draw_count_buffer,
        .draw_info_buffer = m_draw_info_buffer,
        .recheck_instances = m_recheck_instances,
        .recheck_count = m_recheck_count,
    });
}

void deferred_renderer::add_gbuffer_pass(render_graph& graph, bool main_pass)
{
    rdg_scope scope(graph, "GBuffer");

    gbuffer_pass::debug_mode debug_mode = gbuffer_pass::DEBUG_MODE_NONE;
    switch (m_debug_mode)
    {
    case DEBUG_MODE_CLUSTER:
        debug_mode = gbuffer_pass::DEBUG_MODE_CLUSTER;
        break;
    case DEBUG_MODE_CLUSTER_NODE:
        debug_mode = gbuffer_pass::DEBUG_MODE_CLUSTER_NODE;
        break;
    case DEBUG_MODE_TRIANGLE:
        debug_mode = gbuffer_pass::DEBUG_MODE_TRIANGLE;
        break;
    default:
        break;
    }

    graph.add_pass<gbuffer_pass>({
        .draw_buffer = m_draw_buffer,
        .draw_count_buffer = m_draw_count_buffer,
        .draw_info_buffer = m_draw_info_buffer,
        .gbuffers = m_gbuffers,
        .visibility_buffer = m_visibility_buffer,
        .depth_buffer = m_depth_buffer,
        .main_pass = main_pass,
        .debug_mode = debug_mode,
        .debug_output = m_debug_output,
    });
}

void deferred_renderer::add_gtao_pass(render_graph& graph)
{
    auto* gtao = get_feature<gtao_feature>();

    m_ao_buffer = graph.add_texture(
        "AO Buffer",
        m_render_extent,
        RHI_FORMAT_R8_UNORM,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    graph.add_pass<gtao_pass>({
        .slice_count = gtao->slice_count,
        .step_count = gtao->step_count,
        .radius = gtao->radius,
        .falloff = gtao->falloff,
        .hzb = m_tracing_hzb,
        .depth_buffer = m_depth_buffer,
        .normal_buffer = m_gbuffers[GBUFFER_NORMAL],
        .ao_buffer = m_ao_buffer,
    });
}

void deferred_renderer::add_shadow_pass(render_graph& graph)
{
    auto* vsm = get_feature<vsm_feature>();
    auto* shadow = get_feature<shadow_feature>();

    shadow_pass::debug_mode debug_mode = shadow_pass::DEBUG_MODE_NONE;
    switch (m_debug_mode)
    {
    case DEBUG_MODE_VSM_PAGE:
        debug_mode = shadow_pass::DEBUG_MODE_PAGE;
        break;
    case DEBUG_MODE_VSM_PAGE_CACHE:
        debug_mode = shadow_pass::DEBUG_MODE_PAGE_CACHE;
        break;
    case DEBUG_MODE_VSM_CULL:
        debug_mode = shadow_pass::DEBUG_MODE_CULL;
        break;
    default:
        break;
    }

    rdg_buffer* debug_info = nullptr;
    if (vsm->get_debug_info_buffer() != nullptr)
    {
        debug_info = graph.add_buffer("VSM Debug Info", vsm->get_debug_info_buffer());
    }

    graph.add_pass<shadow_pass>({
        .depth_buffer = m_depth_buffer,
        .shadow_light_buffer = m_shadow_light_buffer,
        .vsm_buffer = m_vsm_buffer,
        .virtual_page_table = m_vsm_virtual_page_table,
        .physical_page_table = m_vsm_physical_page_table,
        .physical_shadow_map_static = m_vsm_physical_shadow_map_static,
        .physical_shadow_map_final = m_vsm_physical_shadow_map_final,
        .hzb = m_vsm_hzb,
        .vsm_directional_buffer = m_vsm_directional_buffer,
        .lru_state = graph.add_buffer("VSM LRU State", vsm->get_lru_state()),
        .lru_buffer = graph.add_buffer("VSM LRU Buffer", vsm->get_lru_buffer()),
        .lru_curr_index = vsm->get_curr_lru_index(),
        .lru_prev_index = vsm->get_prev_lru_index(),
        .render_page_budget = shadow->render_page_budget,
        .render_coarse_page = shadow->render_coarse_page,
        .slope_scale_depth_bias = shadow->slope_scale_depth_bias,
        .debug_mode = debug_mode,
        .debug_output = m_debug_output,
        .debug_info = debug_info,
    });
}

void deferred_renderer::add_shading_pass(render_graph& graph)
{
    shading_pass::debug_mode debug_mode = shading_pass::DEBUG_MODE_NONE;
    switch (m_debug_mode)
    {
    case DEBUG_MODE_SHADING_SHADOW_MASK:
        debug_mode = shading_pass::DEBUG_MODE_SHADOW_MASK;
        break;
    default:
        break;
    }

    auto* shadow = get_feature<shadow_feature>();

    graph.add_pass<shading_pass>({
        .gbuffers = m_gbuffers,
        .ao_buffer = m_ao_buffer,
        .depth_buffer = m_depth_buffer,
        .render_target = m_render_target,
        .shadow_light_buffer = m_shadow_light_buffer,
        .vsm_buffer = m_vsm_buffer,
        .vsm_virtual_page_table = m_vsm_virtual_page_table,
        .vsm_physical_shadow_map = m_vsm_physical_shadow_map_final,
        .shadow_sample_mode = static_cast<std::uint32_t>(shadow->sample_mode),
        .shadow_sample_count = shadow->sample_count,
        .shadow_sample_radius = shadow->sample_radius,
        .shadow_normal_bias = shadow->normal_bias,
        .shadow_constant_bias = shadow->constant_bias,
        .prefilter_map = m_prefilter_map,
        .irradiance_sh = m_irradiance_sh,
        .indirect_diffuse = m_indirect_diffuse,
        .debug_mode = debug_mode,
        .debug_output = m_debug_output,
    });
}

void deferred_renderer::add_culling_hzb_pass(render_graph& graph)
{
    graph.add_pass<hzb_pass>({
        .depth_buffer = m_depth_buffer,
        .hzb = m_culling_hzb,
    });
}

void deferred_renderer::add_tracing_hzb_pass(render_graph& graph)
{
    rhi_extent extent = m_depth_buffer->get_extent();

    std::uint32_t hzb_level_count =
        static_cast<std::uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) +
        1;
    hzb_level_count = std::min(hzb_level_count, 5u);

    m_tracing_hzb = graph.add_texture(
        "HZB for Trace",
        extent,
        RHI_FORMAT_R32_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE,
        hzb_level_count);

    graph.add_pass<hzb_pass>({
        .depth_buffer = m_depth_buffer,
        .hzb = m_tracing_hzb,
        .mode = hzb_pass::REDUCTION_MODE_MAX,
    });
}

void deferred_renderer::add_sdf_pass(render_graph& graph)
{
    sdf_pass::debug_mode debug_mode = sdf_pass::DEBUG_MODE_NONE;
    switch (m_debug_mode)
    {
    case DEBUG_MODE_SDF_PAGE:
        debug_mode = sdf_pass::DEBUG_MODE_PAGE;
        break;
    case DEBUG_MODE_SDF_MESH_SDF:
        debug_mode = sdf_pass::DEBUG_MODE_MESH_SDF;
        break;
    default:
        break;
    }

    graph.add_pass<sdf_pass>({
        .depth_buffer = m_depth_buffer,
        .debug_mode = debug_mode,
        .debug_output = m_debug_output,
    });
}

void deferred_renderer::add_ssgi_pass(render_graph& graph)
{
    auto* ssgi = get_feature<ssgi_feature>();

    rhi_extent half_extent = {
        .width = m_render_extent.width / 2,
        .height = m_render_extent.height / 2,
    };

    m_indirect_diffuse = graph.add_texture(
        "Indirect Diffuse",
        half_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    rdg_texture* history = graph.add_texture(
        "SSGI History",
        ssgi->get_history(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    graph.add_pass<ssgi_pass>({
        .scene_color = m_prev_scene_color,
        .scene_color_valid = m_prev_scene_color_valid,
        .motion_vector = m_motion_vectors,
        .normal_buffer = m_gbuffers[GBUFFER_NORMAL],
        .hzb = m_tracing_hzb,
        .irradiance_sh = m_irradiance_sh,
        .indirect_diffuse = m_indirect_diffuse,
        .history = history,
        .history_valid = ssgi->is_history_valid(),
        .bilateral_denoise = ssgi->bilateral_denoise,
        .sample_count = ssgi->sample_count,
        .thickness = ssgi->thickness,
        .iteration_count = ssgi->iteration_count,
        .frame = get_frame(),
        .debug_mode = m_debug_mode == DEBUG_MODE_SSGI ? ssgi_pass::DEBUG_MODE_SSGI :
                                                        ssgi_pass::DEBUG_MODE_NONE,
        .debug_output = m_debug_mode == DEBUG_MODE_SSGI ? m_debug_output : nullptr,
    });
}

void deferred_renderer::add_sky_lut_pass(render_graph& graph)
{
    const auto& context = graph.get_context();
    const auto& environment_module = context.get_module<render_scene_environment>();
    const auto& camera = context.get_camera();

    if (camera.background == BACKGROUND_TYPE_SKYBOX)
    {
        m_prefilter_map = graph.add_texture(
            "Prefilter Map",
            environment_module.get_prefilter_map(),
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        m_irradiance_sh = graph.add_buffer("Irradiance SH", environment_module.get_irradiance_sh());
    }
    else if (camera.background == BACKGROUND_TYPE_ATMOSPHERE)
    {
        auto* atmosphere = get_feature<atmosphere_feature>();

        m_sky_view_lut = graph.add_texture(
            "Sky View LUT",
            {.width = 192, .height = 108},
            RHI_FORMAT_R11G11B10_FLOAT,
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE);

        rhi_extent aerial_perspective_lut_extent;
        if (atmosphere->enable_shadow)
        {
            aerial_perspective_lut_extent = {.width = 200, .height = 150, .depth = 32};
        }
        else
        {
            aerial_perspective_lut_extent = {.width = 32, .height = 32, .depth = 32};
        }

        m_aerial_perspective_lut = graph.add_texture(
            "Aerial Perspective LUT",
            aerial_perspective_lut_extent,
            RHI_FORMAT_R16G16B16A16_FLOAT,
            RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE);

        if (environment_module.is_atmosphere_dirty())
        {
            m_ibl_dirty = true;
        }

        atmosphere_lut_pass::parameter parameter = {
            .sky_view_lut = m_sky_view_lut,
            .aerial_perspective_lut = m_aerial_perspective_lut,
            .enable_multi_scattering = atmosphere->enable_multi_scattering,
        };

        if (atmosphere->enable_shadow)
        {
            parameter.vsm_buffer = m_vsm_buffer;
            parameter.vsm_virtual_page_table = m_vsm_virtual_page_table;
            parameter.vsm_physical_shadow_map = m_vsm_physical_shadow_map_final;
        }

        const auto& camera_module = context.get_module<render_scene_camera>();
        const auto& camera_data = camera_module.get_camera(camera.id);

        if (m_ibl_dirty && get_frame() % atmosphere->ibl_update_interval == 0)
        {
            m_prefilter_map = graph.add_texture(
                "Prefilter Map",
                camera_data.prefilter_map.get(),
                RHI_TEXTURE_LAYOUT_UNDEFINED,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
            m_irradiance_sh = graph.add_buffer("Irradiance SH", camera_data.irradiance_sh.get());

            parameter.prefilter_map = m_prefilter_map;
            parameter.irradiance_sh = m_irradiance_sh;

            m_ibl_dirty = false;
        }
        else
        {
            m_prefilter_map = graph.add_texture(
                "Prefilter Map",
                camera_data.prefilter_map.get(),
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
            m_irradiance_sh = graph.add_buffer("Irradiance SH", camera_data.irradiance_sh.get());
        }

        graph.add_pass<atmosphere_lut_pass>(parameter);
    }
}

void deferred_renderer::add_sky_pass(render_graph& graph)
{
    const auto& context = graph.get_context();
    const auto& mesh_module = context.get_module<render_scene_mesh>();

    switch (context.get_camera().background)
    {
    case BACKGROUND_TYPE_SKYBOX: {
        graph.add_pass<skybox_pass>({
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = mesh_module.get_instance_count() == 0,
        });
        break;
    }
    case BACKGROUND_TYPE_ATMOSPHERE: {
        graph.add_pass<atmosphere_pass>({
            .sky_view_lut = m_sky_view_lut,
            .aerial_perspective_lut = m_aerial_perspective_lut,
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = mesh_module.get_instance_count() == 0,
        });
        break;
    }
    default:
        break;
    }
}

void deferred_renderer::add_dithering_pass(render_graph& graph)
{
    auto* dithering = get_feature<dithering_feature>(true);
    if (dithering == nullptr)
    {
        return;
    }

    graph.add_pass<dithering_pass>({
        .render_target = m_render_target,
        .frame = get_frame(),
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
    graph.add_pass<taa_pass>({
        .current_render_target = m_render_target,
        .history_render_target = m_prev_scene_color,
        .depth_buffer = m_depth_buffer,
        .motion_vector = m_motion_vectors,
        .history_valid = m_prev_scene_color_valid,
    });
}

void deferred_renderer::add_eye_adaptation_pass(render_graph& graph)
{
    auto* eye_adaptation = get_feature<eye_adaptation_feature>(true);
    if (eye_adaptation == nullptr)
    {
        return;
    }

    rdg_texture* exposure = graph.add_texture(
        "Exposure",
        eye_adaptation->get_exposure(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    graph.add_pass<eye_adaptation_pass>({
        .render_target = m_render_target,
        .exposure = exposure,
        .min_ev = eye_adaptation->min_ev,
        .max_ev = eye_adaptation->max_ev,
        .low_percent = eye_adaptation->low_percent,
        .high_percent = eye_adaptation->high_percent,
        .min_brightness = eye_adaptation->min_brightness,
        .max_brightness = eye_adaptation->max_brightness,
        .speed_down = eye_adaptation->speed_down,
        .speed_up = eye_adaptation->speed_up,
        .delta_time = get_delta_time(),
        .debug_output = m_debug_mode == DEBUG_MODE_EYE_ADAPTATION ? m_debug_output : nullptr,
    });
}

void deferred_renderer::add_bloom_pass(render_graph& graph)
{
    auto* bloom = get_feature<bloom_feature>(true);
    if (bloom == nullptr)
    {
        return;
    }

    bloom_pass::debug_mode debug_mode = bloom_pass::DEBUG_MODE_NONE;
    switch (m_debug_mode)
    {
    case DEBUG_MODE_BLOOM:
        debug_mode = bloom_pass::DEBUG_MODE_BLOOM;
        break;
    case DEBUG_MODE_BLOOM_PREFILTER:
        debug_mode = bloom_pass::DEBUG_MODE_PREFILTER;
        break;
    default:
        break;
    }

    graph.add_pass<bloom_pass>({
        .render_target = m_render_target,
        .threshold = bloom->threshold,
        .intensity = bloom->intensity,
        .knee = bloom->knee,
        .radius = bloom->radius,
        .debug_mode = debug_mode,
        .debug_output = m_debug_output,
    });
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
        graph.get_context().get_render_target(),
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