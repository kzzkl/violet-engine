#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class deferred_renderer : public renderer
{
public:
    enum debug_mode
    {
        DEBUG_MODE_NONE,
        DEBUG_MODE_CLUSTER,
        DEBUG_MODE_CLUSTER_NODE,
        DEBUG_MODE_TRIANGLE,
        DEBUG_MODE_VSM_PAGE,
        DEBUG_MODE_VSM_PAGE_CACHE,
        DEBUG_MODE_SHADING_SHADOW_MASK,
        DEBUG_MODE_BLOOM,
        DEBUG_MODE_BLOOM_PREFILTER,
    };

    deferred_renderer();

    void set_debug_mode(debug_mode mode) noexcept
    {
        m_debug_mode = mode;
    }

    debug_mode get_debug_mode() const noexcept
    {
        return m_debug_mode;
    }

protected:
    void on_render(render_graph& graph) override;

    rdg_texture* get_render_target() const noexcept
    {
        return m_render_target;
    }

private:
    void prepare(render_graph& graph);
    void add_cull_pass(render_graph& graph, bool main_pass);
    void add_gbuffer_pass(render_graph& graph, bool main_pass);
    void add_hzb_pass(render_graph& graph);
    void add_gtao_pass(render_graph& graph);
    void add_shadow_pass(render_graph& graph);
    void add_shading_pass(render_graph& graph);
    void add_skybox_pass(render_graph& graph);
    void add_motion_vector_pass(render_graph& graph);
    void add_taa_pass(render_graph& graph);
    void add_bloom_pass(render_graph& graph);
    void add_tone_mapping_pass(render_graph& graph);
    void add_present_pass(render_graph& graph);

    rhi_texture_extent m_render_extent;

    std::vector<rdg_texture*> m_gbuffers;
    rdg_texture* m_visibility_buffer{nullptr};
    rdg_texture* m_depth_buffer;
    rdg_texture* m_ao_buffer;

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_hzb{nullptr};

    rdg_texture* m_motion_vectors{nullptr};

    rdg_buffer* m_cluster_queue;
    rdg_buffer* m_cluster_queue_state;
    rdg_buffer* m_recheck_instances;

    rdg_buffer* m_draw_buffer{nullptr};
    rdg_buffer* m_draw_count_buffer{nullptr};
    rdg_buffer* m_draw_info_buffer{nullptr};

    rdg_buffer* m_vsm_buffer{nullptr};
    rdg_buffer* m_vsm_virtual_page_table{nullptr};
    rdg_buffer* m_vsm_physical_page_table{nullptr};
    rdg_texture* m_vsm_physical_shadow_map_static{nullptr};
    rdg_texture* m_vsm_physical_shadow_map_final{nullptr};
    rdg_texture* m_vsm_hzb{nullptr};
    rdg_buffer* m_vsm_directional_buffer{nullptr};

    debug_mode m_debug_mode{DEBUG_MODE_NONE};
    rdg_texture* m_debug_output{nullptr};
};
} // namespace violet