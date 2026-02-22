#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class shading_pass
{
public:
    enum debug_mode
    {
        DEBUG_MODE_NONE,
        DEBUG_MODE_SHADOW_MASK,
    };

    struct parameter
    {
        std::span<rdg_texture*> gbuffers;
        std::span<rdg_texture*> auxiliary_buffers;
        rdg_texture* render_target;

        rdg_buffer* vsm_buffer{nullptr};
        rdg_buffer* vsm_virtual_page_table{nullptr};
        rdg_texture* vsm_physical_shadow_map{nullptr};
        rdg_buffer* vsm_directional_buffer{nullptr};

        std::uint32_t shadow_sample_mode; // 0: none, 1: PCF, 2: PCSS
        std::uint32_t shadow_sample_count;
        float shadow_sample_radius;

        float shadow_normal_offset;
        float shadow_constant_bias;
        float shadow_receiver_plane_bias;

        debug_mode debug_mode{DEBUG_MODE_NONE};
        std::uint32_t debug_light_id{0};
        rdg_texture* debug_output{nullptr};
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    static constexpr std::uint32_t tile_size = 16;

    enum lighting_stage
    {
        LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED,
        LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED,
        LIGHTING_STAGE_INDIRECT_LIGHTING,
    };

    void add_clear_pass(render_graph& graph, const parameter& parameter);
    void add_tile_classify_pass(render_graph& graph, const parameter& parameter);
    void add_tile_shading_pass(
        render_graph& graph,
        const parameter& parameter,
        lighting_stage stage,
        std::uint32_t light_id = 0) const;

    void add_shadow_mask_pass(
        render_graph& graph,
        const parameter& parameter,
        std::uint32_t light_id);

    void add_debug_pass(render_graph& graph, const parameter& parameter);

    rdg_buffer* m_worklist_buffer{nullptr};
    rdg_buffer* m_shading_dispatch_buffer{nullptr};

    std::uint32_t m_tile_count;
    std::uint32_t m_shading_model_count{0};

    rdg_texture* m_shadow_mask{nullptr};
};
} // namespace violet