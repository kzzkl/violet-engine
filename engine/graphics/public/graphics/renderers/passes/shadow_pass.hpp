#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class shadow_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        rdg_buffer* vsm_buffer{nullptr};
        rdg_buffer* vsm_virtual_page_table{nullptr};
        rdg_buffer* vsm_physical_page_table{nullptr};
        rdg_texture* vsm_physical_texture{nullptr};

        rdg_buffer* lru_state;
        rdg_buffer* lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t lru_prev_index;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void prepare(render_graph& graph);
    void light_cull(render_graph& graph);
    void clear_page_table(render_graph& graph);
    void mark_visible_pages(render_graph& graph);
    void mark_resident_pages(render_graph& graph);
    void update_lru(render_graph& graph);
    void allocate_pages(render_graph& graph);
    void clear_physical_page(render_graph& graph);
    void update_projection(render_graph& graph);
    void instance_cull(render_graph& graph);
    void render_shadow(render_graph& graph);

    void add_debug_pass(render_graph& graph);

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rdg_buffer* m_virtual_page_dispatch_buffer{nullptr};

    rdg_buffer* m_visible_light_count{nullptr};
    rdg_buffer* m_visible_light_ids{nullptr};
    rdg_buffer* m_visible_vsm_ids{nullptr};

    rdg_buffer* m_vsm_buffer{nullptr};
    rdg_buffer* m_vsm_virtual_page_table{nullptr};
    rdg_buffer* m_vsm_physical_page_table{nullptr};
    rdg_texture* m_vsm_physical_texture{nullptr};

    rdg_buffer* m_vsm_projection_buffer{nullptr};
    rdg_buffer* m_clear_physical_page_dispatch_buffer{nullptr};

    rdg_buffer* m_lru_state{nullptr};
    rdg_buffer* m_lru_buffer{nullptr};
    std::uint32_t m_lru_curr_index;
    std::uint32_t m_lru_prev_index;

    rdg_buffer* m_draw_buffer{nullptr};
    rdg_buffer* m_draw_count_buffer{nullptr};
    rdg_buffer* m_draw_info_buffer{nullptr};

    // rdg_texture* m_debug_texture{nullptr};
};
} // namespace violet