#pragma once

#include "graphics/renderer.hpp"
#include "sample/imgui_pass.hpp"

namespace violet
{
class mmd_renderer : public renderer
{
public:
    mmd_renderer();

private:
    void on_render(render_graph& graph) override;

    void add_cull_pass(render_graph& graph, bool main_pass);
    void add_gbuffer_pass(render_graph& graph, bool main_pass);
    void add_hzb_pass(render_graph& graph);
    void add_gtao_pass(render_graph& graph);
    void add_shading_pass(render_graph& graph);
    void add_skybox_pass(render_graph& graph);
    void add_transparent_pass(render_graph& graph);
    void add_motion_vector_pass(render_graph& graph);
    void add_taa_pass(render_graph& graph);
    void add_tone_mapping_pass(render_graph& graph);
    void add_present_pass(render_graph& graph);

    rhi_texture_extent m_render_extent;

    std::vector<rdg_texture*> m_gbuffers;
    rdg_texture* m_visibility_buffer{nullptr};
    rdg_texture* m_depth_buffer{nullptr};
    rdg_texture* m_ao_buffer{nullptr};

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_hzb{nullptr};

    rdg_texture* m_motion_vectors{nullptr};

    rdg_buffer* m_recheck_instances{nullptr};

    rdg_buffer* m_draw_buffer{nullptr};
    rdg_buffer* m_draw_count_buffer{nullptr};
    rdg_buffer* m_draw_info_buffer{nullptr};

    imgui_pass m_imgui_pass;
};
} // namespace violet