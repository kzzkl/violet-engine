#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class shadow_pass
{
public:
    enum debug_mode
    {
        DEBUG_MODE_NONE,
        DEBUG_MODE_PAGE,
        DEBUG_MODE_PAGE_CACHE,
        DEBUG_MODE_PHYSICAL_PAGE_TABLE,
        DEBUG_MODE_CULL,
    };

    struct parameter
    {
        rdg_texture* depth_buffer;

        rdg_buffer* shadow_light_buffer;

        rdg_buffer* vsm_buffer{nullptr};
        rdg_buffer* virtual_page_table{nullptr};
        rdg_buffer* physical_page_table{nullptr};
        rdg_texture* physical_shadow_map_static{nullptr};
        rdg_texture* physical_shadow_map_final{nullptr};
        rdg_texture* hzb{nullptr};
        rdg_buffer* vsm_directional_buffer{nullptr};

        rdg_buffer* lru_state;
        rdg_buffer* lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t lru_prev_index;

        std::uint32_t render_page_budget{0};
        bool render_coarse_page{false};

        float slope_scale_depth_bias;

        debug_mode debug_mode{DEBUG_MODE_NONE};
        std::uint32_t debug_light_id{0};
        rdg_texture* debug_output{nullptr};
        rdg_buffer* debug_info{nullptr};
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void prepare(render_graph& graph, const parameter& parameter);
    void light_cull(render_graph& graph, const parameter& parameter);
    void clear_page_table(render_graph& graph, const parameter& parameter);
    void mark_visible_pages(render_graph& graph, const parameter& parameter);
    void mark_coarse_pages(render_graph& graph, const parameter& parameter);
    void mark_resident_pages(render_graph& graph, const parameter& parameter);
    void mark_cache_dirty_pages(render_graph& graph, const parameter& parameter);
    void build_dispatch_args(render_graph& graph, const parameter& parameter);
    void update_lru(render_graph& graph, const parameter& parameter);
    void allocate_pages(render_graph& graph, const parameter& parameter);
    void mark_fallback_pages(render_graph& graph, const parameter& parameter);
    void clear_physical_pages(render_graph& graph, const parameter& parameter);

    void instance_cull(render_graph& graph, const parameter& parameter);
    void prepare_cluster_cull(render_graph& graph, rdg_buffer* dispatch_buffer, bool cull_cluster);
    void cluster_cull(render_graph& graph, const parameter& parameter);

    void render_shadow(
        render_graph& graph,
        const parameter& parameter,
        bool opacity_cutoff,
        rhi_cull_mode cull_mode);
    void merge_physical_pages(render_graph& graph, const parameter& parameter);

    void build_hzb(render_graph& graph, const parameter& parameter);

    void add_debug_pass(render_graph& graph, const parameter& parameter);

    rdg_buffer* m_virtual_page_indirect_args{nullptr};

    rdg_buffer* m_visible_virtual_page_list{nullptr};
    rdg_buffer* m_visible_virtual_page_indirect_args{nullptr};
    rdg_buffer* m_visible_virtual_page_texels_indirect_args{nullptr};

    rdg_buffer* m_render_physical_page_list{nullptr};
    rdg_buffer* m_render_physical_page_texels_indirect_args{nullptr};

    rdg_buffer* m_vsm_info{nullptr};
    rdg_buffer* m_visible_light_list{nullptr};
    rdg_buffer* m_visible_vsm_list{nullptr};

    rdg_buffer* m_vsm_bounds_buffer{nullptr};

    rdg_buffer* m_draw_buffer{nullptr};
    rdg_buffer* m_draw_count_buffer{nullptr};
    rdg_buffer* m_draw_info_buffer{nullptr};

    rdg_buffer* m_cluster_queue{nullptr};
    rdg_buffer* m_cluster_queue_state{nullptr};

    rhi_sampler* m_hzb_sampler{nullptr};
};
} // namespace violet