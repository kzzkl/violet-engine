#pragma once

#include "graphics/renderer.hpp"
#include "imgui_pass.hpp"

namespace violet
{
class mmd_renderer : public renderer
{
public:
    void render(render_graph& graph) override;

private:
    void add_cull_pass(render_graph& graph);
    void add_mesh_pass(render_graph& graph);
    void add_skybox_pass(render_graph& graph);
    void add_motion_vector_pass(render_graph& graph);
    void add_taa_pass(render_graph& graph);
    void add_tone_mapping_pass(render_graph& graph);
    void add_present_pass(render_graph& graph);

    rhi_texture_extent m_render_extent;

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rdg_texture* m_gbuffer_normal{nullptr};
    rdg_texture* m_gbuffer_emissive{nullptr};

    rdg_texture* m_motion_vectors{nullptr};

    rdg_buffer* m_command_buffer{nullptr};
    rdg_buffer* m_count_buffer{nullptr};

    imgui_pass m_imgui_pass;
};
} // namespace violet