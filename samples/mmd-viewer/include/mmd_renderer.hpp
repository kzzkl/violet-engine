#pragma once

#include "graphics/renderer.hpp"
#include "imgui_pass.hpp"

namespace violet
{
class mmd_renderer : public renderer
{
public:
    void render(render_graph& graph, const render_scene& scene, const render_camera& camera)
        override;

private:
    void add_cull_pass(render_graph& graph, const render_scene& scene, const render_camera& camera);
    void add_mesh_pass(render_graph& graph, const render_scene& scene, const render_camera& camera);
    void add_lighting_pass(
        render_graph& graph,
        const render_scene& scene,
        const render_camera& camera);
    void add_skybox_pass(
        render_graph& graph,
        const render_scene& scene,
        const render_camera& camera);
    void add_tone_mapping_pass(render_graph& graph);
    void add_present_pass(render_graph& graph, const render_camera& camera);

    rhi_texture_extent m_render_extent;

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rdg_texture* m_gbuffer_albedo;
    rdg_texture* m_gbuffer_material;
    rdg_texture* m_gbuffer_normal;
    rdg_texture* m_gbuffer_emissive;

    rdg_buffer* m_command_buffer{nullptr};
    rdg_buffer* m_count_buffer{nullptr};

    imgui_pass m_imgui_pass;
};
} // namespace violet