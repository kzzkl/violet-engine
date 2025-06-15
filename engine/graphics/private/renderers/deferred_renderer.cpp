#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/renderers/features/cluster_render_feature.hpp"
#include "graphics/renderers/features/gtao_render_feature.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"
#include "graphics/renderers/passes/copy_depth_pass.hpp"
#include "graphics/renderers/passes/cull_pass.hpp"
#include "graphics/renderers/passes/gtao_pass.hpp"
#include "graphics/renderers/passes/hzb_pass.hpp"
#include "graphics/renderers/passes/lighting/physical_pass.hpp"
#include "graphics/renderers/passes/lighting/unlit_pass.hpp"
#include "graphics/renderers/passes/mesh_pass.hpp"
#include "graphics/renderers/passes/motion_vector_pass.hpp"
#include "graphics/renderers/passes/skybox_pass.hpp"
#include "graphics/renderers/passes/taa_pass.hpp"
#include "graphics/renderers/passes/tone_mapping_pass.hpp"

namespace violet
{
deferred_renderer::deferred_renderer()
{
    add_feature<taa_render_feature>();
    add_feature<gtao_render_feature>();
    add_feature<cluster_render_feature>();
}

void deferred_renderer::on_render(render_graph& graph)
{
    m_render_extent = graph.get_camera().get_render_target()->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        m_render_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_SRC);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        m_render_extent,
        RHI_FORMAT_D32_FLOAT_S8_UINT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE);

    m_hzb = graph.add_texture(
        "HZB",
        graph.get_camera().get_hzb(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    if (graph.get_scene().get_instance_count() != 0)
    {
        add_cull_pass(graph);
        add_mesh_pass(graph);
        add_hzb_pass(graph);

        if (get_feature<gtao_render_feature>(true))
        {
            add_gtao_pass(graph);
        }
        else
        {
            m_ao_buffer = nullptr;
        }

        add_lighting_pass(graph);
    }

    if (graph.get_scene().has_skybox())
    {
        add_skybox_pass(graph);
    }

    if (get_feature<taa_render_feature>(true))
    {
        add_motion_vector_pass(graph);
        add_taa_pass(graph);
    }

    add_tone_mapping_pass(graph);
    add_present_pass(graph);
}

void deferred_renderer::add_cull_pass(render_graph& graph)
{
    auto result = cull_pass::add(
        graph,
        {
            .hzb = m_hzb,
            .cluster_feature = get_feature<cluster_render_feature>(true),
        });

    m_command_buffer = result.command_buffer;
    m_count_buffer = result.count_buffer;
}

void deferred_renderer::add_mesh_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Mesh");

    m_gbuffer_albedo = graph.add_texture(
        "GBuffer Albedo",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_gbuffer_material = graph.add_texture(
        "GBuffer Material",
        m_render_extent,
        RHI_FORMAT_R8G8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_gbuffer_normal = graph.add_texture(
        "GBuffer Normal",
        m_render_extent,
        RHI_FORMAT_R16G16_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_gbuffer_emissive = graph.add_texture(
        "GBuffer Emissive",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    std::vector<rdg_texture*> render_targets = {
        m_gbuffer_albedo,
        m_gbuffer_material,
        m_gbuffer_normal,
        m_gbuffer_emissive,
    };

    mesh_pass::add(
        graph,
        {
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
            .render_targets = render_targets,
            .depth_buffer = m_depth_buffer,
            .material_type = MATERIAL_OPAQUE,
            .clear = true,
        });
}

void deferred_renderer::add_hzb_pass(render_graph& graph)
{
    hzb_pass::add(
        graph,
        {
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

    gtao_pass::add(
        graph,
        {
            .slice_count = gtao->get_slice_count(),
            .step_count = gtao->get_step_count(),
            .radius = gtao->get_radius(),
            .falloff = gtao->get_falloff(),
            .hzb = m_hzb,
            .depth_buffer = m_depth_buffer,
            .normal_buffer = m_gbuffer_normal,
            .ao_buffer = m_ao_buffer,
        });
}

void deferred_renderer::add_lighting_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Lighting");

    rdg_texture* depth_copy = graph.add_texture(
        "Depth Copy",
        m_render_extent,
        RHI_FORMAT_R32_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    copy_depth_pass::add(
        graph,
        {
            .src = m_depth_buffer,
            .dst = depth_copy,
        });

    unlit_pass::add(
        graph,
        {
            .gbuffer_albedo = m_gbuffer_albedo,
            .depth_buffer = m_depth_buffer,
            .render_target = m_render_target,
            .clear = true,
        });

    physical_pass::add(
        graph,
        {
            .gbuffer_albedo = m_gbuffer_albedo,
            .gbuffer_material = m_gbuffer_material,
            .gbuffer_normal = m_gbuffer_normal,
            .gbuffer_depth = depth_copy,
            .gbuffer_emissive = m_gbuffer_emissive,
            .ao_buffer = m_ao_buffer,
            .depth_buffer = m_depth_buffer,
            .render_target = m_render_target,
            .clear = false,
        });
}

void deferred_renderer::add_skybox_pass(render_graph& graph)
{
    skybox_pass::add(
        graph,
        {
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = graph.get_scene().get_instance_count() == 0,
        });
}

void deferred_renderer::add_motion_vector_pass(render_graph& graph)
{
    m_motion_vectors = graph.add_texture(
        "Motion Vectors",
        m_render_extent,
        RHI_FORMAT_R16G16_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    motion_vector_pass::add(
        graph,
        {
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

    taa_pass::add(
        graph,
        {
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

    tone_mapping_pass::add(
        graph,
        {
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

    blit_pass::add(
        graph,
        {
            .src = m_render_target,
            .src_region = region,
            .dst = camera_output,
            .dst_region = region,
        });

    m_render_target = camera_output;
}
} // namespace violet