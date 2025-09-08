#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class gbuffer_pass
{
public:
    struct parameter
    {
        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;

        std::span<rdg_texture*> gbuffers;
        rdg_texture* visibility_buffer;
        rdg_texture* depth_buffer;

        bool clear;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    static constexpr std::uint32_t tile_size = 8;

    void add_clear_pass(render_graph& graph, const parameter& parameter);

    void add_visibility_pass(render_graph& graph, const parameter& parameter);
    void add_material_classify_pass(render_graph& graph);
    void add_material_resolve_pass(
        render_graph& graph,
        const parameter& parameter,
        std::uint32_t material_index,
        const rdg_compute_pipeline& pipeline);

    void add_deferred_pass(render_graph& graph, const parameter& parameter);

    rdg_texture* m_visibility_buffer{nullptr};
    rdg_buffer* m_worklist_buffer{nullptr};
    rdg_buffer* m_worklist_size_buffer{nullptr};
    rdg_buffer* m_material_offset_buffer{nullptr};
    rdg_buffer* m_resolve_dispatch_buffer{nullptr};

    std::uint32_t m_tile_count;
    std::uint32_t m_material_count{0};
};
} // namespace violet