#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class bloom_pass
{
public:
    enum debug_mode
    {
        DEBUG_MODE_NONE,
        DEBUG_MODE_BLOOM,
        DEBUG_MODE_PREFILTER,
    };

    struct parameter
    {
        rdg_texture* render_target;

        float threshold;
        float intensity;
        float knee;
        float radius;

        debug_mode debug_mode{DEBUG_MODE_NONE};
        rdg_texture* debug_output{nullptr};
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    static constexpr std::uint32_t bloom_mip_count = 5;

    void prefilter(render_graph& graph, const parameter& parameter);
    void downsample(render_graph& graph);
    void blur(render_graph& graph);
    void upsample(render_graph& graph, const parameter& parameter);
    void merge(render_graph& graph, const parameter& parameter);

    void debug(render_graph& graph, const parameter& parameter);

    rdg_texture* m_downsample_chain;
    rdg_texture* m_upsample_chain;
};
} // namespace violet
