#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class bloom_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;

        float threshold;
        float intensity;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    static constexpr std::uint32_t bloom_mip_count = 5;

    void prefilter(render_graph& graph, const parameter& parameter);
    void downsample(render_graph& graph);
    void blur(render_graph& graph);
    void upsample(render_graph& graph);
    void merge(render_graph& graph, const parameter& parameter);

    rdg_texture* m_downsample_chain;
    rdg_texture* m_upsample_chain;
};
} // namespace violet
