#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/passes/blit_pass.hpp"
#include "graphics/passes/copy_depth_pass.hpp"
#include "graphics/passes/cull_pass.hpp"
#include "graphics/passes/lighting/physical_pass.hpp"
#include "graphics/passes/lighting/unlit_pass.hpp"
#include "graphics/passes/mesh_pass.hpp"
#include "graphics/passes/skybox_pass.hpp"
#include "graphics/passes/tone_mapping_pass.hpp"

namespace violet
{
void deferred_renderer::render(
    render_graph& graph,
    const render_scene& scene,
    const render_camera& camera)
{
    m_render_extent = camera.render_targets[0]->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R16G16B16A16_FLOAT,
            .flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE,
        });

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_D32_FLOAT_S8_UINT,
            .flags = RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE,
        });

    if (scene.get_instance_count() != 0)
    {
        add_cull_pass(graph, scene, camera);
        add_mesh_pass(graph, scene, camera);
        add_lighting_pass(graph, scene, camera);
    }

    if (scene.has_skybox())
    {
        add_skybox_pass(graph, scene, camera);
    }

    add_tone_mapping_pass(graph);
    add_present_pass(graph, camera);
}

void deferred_renderer::add_cull_pass(
    render_graph& graph,
    const render_scene& scene,
    const render_camera& camera)
{
    rdg_scope scope(graph, "Cull");

    m_command_buffer = graph.add_buffer(
        "Command Buffer",
        {
            .data = nullptr,
            .size = scene.get_instance_capacity() * sizeof(shader::draw_command),
            .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT,
        });

    m_count_buffer = graph.add_buffer(
        "Count Buffer",
        {
            .data = nullptr,
            .size = scene.get_group_capacity() * sizeof(std::uint32_t),
            .flags = RHI_BUFFER_STORAGE_TEXEL | RHI_BUFFER_INDIRECT | RHI_BUFFER_TRANSFER_DST,
            .texel =
                {
                    .format = RHI_FORMAT_R32_UINT,
                },
        });

    cull_pass::add(
        graph,
        {
            .scene = scene,
            .camera = camera,
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
        });
}

void deferred_renderer::add_mesh_pass(
    render_graph& graph,
    const render_scene& scene,
    const render_camera& camera)
{
    rdg_scope scope(graph, "Mesh");

    m_gbuffer_albedo = graph.add_texture(
        "GBuffer Albedo",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R8G8B8A8_UNORM,
            .flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE,
        });

    m_gbuffer_material = graph.add_texture(
        "GBuffer Material",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R8G8_UNORM,
            .flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE,
        });

    m_gbuffer_normal = graph.add_texture(
        "GBuffer Normal",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R16G16_UNORM,
            .flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE,
        });

    m_gbuffer_emissive = graph.add_texture(
        "GBuffer Emissive",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R8G8B8A8_UNORM,
            .flags = RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE,
        });

    mesh_pass::add(
        graph,
        {
            .scene = scene,
            .camera = camera,
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
            .gbuffer_albedo = m_gbuffer_albedo,
            .gbuffer_material = m_gbuffer_material,
            .gbuffer_normal = m_gbuffer_normal,
            .gbuffer_emissive = m_gbuffer_emissive,
            .depth_buffer = m_depth_buffer,
            .clear = true,
        });
}

void deferred_renderer::add_lighting_pass(
    render_graph& graph,
    const render_scene& scene,
    const render_camera& camera)
{
    rdg_scope scope(graph, "Lighting");

    rdg_texture* depth_copy = graph.add_texture(
        "Depth Copy",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R32_FLOAT,
            .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
        });

    rhi_texture_region region = {
        .offset_x = 0,
        .offset_y = 0,
        .extent = m_render_extent,
        .level = 0,
        .layer = 0,
        .layer_count = 1,
    };

    copy_depth_pass::add(
        graph,
        {
            .src = m_depth_buffer,
            .dst = depth_copy,
        });

    unlit_pass::add(
        graph,
        {
            .scene = scene,
            .gbuffer_albedo = m_gbuffer_albedo,
            .depth_buffer = m_depth_buffer,
            .render_target = m_render_target,
            .clear = true,
        });

    physical_pass::add(
        graph,
        {
            .scene = scene,
            .camera = camera,
            .gbuffer_albedo = m_gbuffer_albedo,
            .gbuffer_material = m_gbuffer_material,
            .gbuffer_normal = m_gbuffer_normal,
            .gbuffer_depth = depth_copy,
            .gbuffer_emissive = m_gbuffer_emissive,
            .depth_buffer = m_depth_buffer,
            .render_target = m_render_target,
            .clear = false,
        });
}

void deferred_renderer::add_skybox_pass(
    render_graph& graph,
    const render_scene& scene,
    const render_camera& camera)
{
    skybox_pass::add(
        graph,
        {
            .scene = scene,
            .camera = camera,
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = scene.get_instance_count() == 0,
        });
}

void deferred_renderer::add_tone_mapping_pass(render_graph& graph)
{
    rdg_texture* ldr_target = graph.add_texture(
        "LDR Target",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R8G8B8A8_UNORM,
            .flags = RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_STORAGE,
        });

    tone_mapping_pass::add(
        graph,
        {
            .hdr_texture = m_render_target,
            .ldr_texture = ldr_target,
        });

    m_render_target = ldr_target;
}

void deferred_renderer::add_present_pass(render_graph& graph, const render_camera& camera)
{
    rdg_texture* camera_output = graph.add_texture(
        "Camera Output",
        camera.render_targets[0],
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
}
} // namespace violet