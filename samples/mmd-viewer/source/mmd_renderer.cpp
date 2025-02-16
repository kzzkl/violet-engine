#include "mmd_renderer.hpp"
#include "graphics/passes/blit_pass.hpp"
#include "graphics/passes/cull_pass.hpp"
#include "graphics/passes/mesh_pass.hpp"
#include "graphics/passes/motion_vector_pass.hpp"
#include "graphics/passes/skybox_pass.hpp"
#include "graphics/passes/taa_pass.hpp"
#include "graphics/passes/tone_mapping_pass.hpp"

namespace violet
{
void mmd_renderer::render(render_graph& graph)
{
    m_render_extent = graph.get_camera().get_render_target()->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        m_render_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        m_render_extent,
        RHI_FORMAT_D32_FLOAT_S8_UINT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE);

    if (graph.get_scene().get_instance_count() != 0)
    {
        add_cull_pass(graph);
        add_mesh_pass(graph);
    }

    if (graph.get_scene().has_skybox())
    {
        add_skybox_pass(graph);
    }

    if (graph.get_camera().get_feature<taa_render_feature>())
    {
        add_motion_vector_pass(graph);
        add_taa_pass(graph);
    }

    add_tone_mapping_pass(graph);
    add_present_pass(graph);

    m_imgui_pass.add(
        graph,
        {
            .render_target = m_render_target,
        });
}

void mmd_renderer::add_cull_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Cull");

    m_command_buffer = graph.add_buffer(
        "Command Buffer",
        graph.get_scene().get_instance_capacity() * sizeof(shader::draw_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_count_buffer = graph.add_buffer(
        "Count Buffer",
        graph.get_scene().get_group_capacity() * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE_TEXEL | RHI_BUFFER_INDIRECT | RHI_BUFFER_TRANSFER_DST);

    cull_pass::add(
        graph,
        {
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
        });
}

void mmd_renderer::add_mesh_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Mesh");

    m_render_target = graph.add_texture(
        "GBuffer Albedo",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
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
        m_render_target,
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

void mmd_renderer::add_skybox_pass(render_graph& graph)
{
    skybox_pass::add(
        graph,
        {
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = graph.get_scene().get_instance_count() == 0,
        });
}

void mmd_renderer::add_motion_vector_pass(render_graph& graph)
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

void mmd_renderer::add_taa_pass(render_graph& graph)
{
    auto* taa = graph.get_camera().get_feature<taa_render_feature>();

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

void mmd_renderer::add_tone_mapping_pass(render_graph& graph)
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

void mmd_renderer::add_present_pass(render_graph& graph)
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