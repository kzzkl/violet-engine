#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class shading_pass
{
public:
    struct parameter
    {
        std::span<rdg_texture*> gbuffers;
        std::span<rdg_texture*> auxiliary_buffers;
        rdg_texture* render_target;

        rdg_buffer* vsm_buffer{nullptr};
        rdg_buffer* vsm_virtual_page_table{nullptr};
        rdg_texture* vsm_physical_texture{nullptr};
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    static constexpr std::uint32_t tile_size = 16;

    void add_clear_pass(render_graph& graph, const parameter& parameter);
    void add_tile_classify_pass(render_graph& graph, const parameter& parameter);
    void add_tile_shading_pass(render_graph& graph, const parameter& parameter) const;

    rdg_buffer* m_worklist_buffer{nullptr};
    rdg_buffer* m_shading_dispatch_buffer{nullptr};

    std::uint32_t m_tile_count;
    std::uint32_t m_shading_model_count{0};
};
} // namespace violet