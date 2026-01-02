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
    void add_debug_pass(render_graph& graph);

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rdg_buffer* m_visible_light_count{nullptr};
    rdg_buffer* m_visible_light_ids{nullptr};
    rdg_buffer* m_visible_vsm_ids{nullptr};
    
    rdg_buffer* m_directional_vsm_buffer{nullptr};

    rdg_buffer* m_vsm_buffer{nullptr};
    rdg_buffer* m_virtual_page_table{nullptr};
    rdg_buffer* m_physical_page_table{nullptr};

    rdg_buffer* m_lru_state{nullptr};
    rdg_buffer* m_lru_buffer{nullptr};
    std::uint32_t m_lru_curr_index;
    std::uint32_t m_lru_prev_index;

    rdg_buffer* m_vsm_dispatch_buffer{nullptr};
};
} // namespace violet