#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class cluster_render_feature;
class cull_pass
{
public:
    struct parameter
    {
        rdg_texture* hzb;
        cluster_render_feature* cluster_feature;
    };

    struct output
    {
        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;
    };

    output add(render_graph& graph, const parameter& parameter);

private:
    void prepare_cluster_queue(render_graph& graph);

    void prepare_cluster_cull(render_graph& graph, rdg_buffer* dispatch_buffer, bool cull_cluster);

    void add_prepare_pass(render_graph& graph);

    void add_instance_cull_pass(render_graph& graph);
    void add_cluster_cull_pass(
        render_graph& graph,
        std::uint32_t max_cluster_count,
        std::uint32_t max_cluster_node_count);
    void add_cluster_cull_pass_persistent(
        render_graph& graph,
        std::uint32_t max_cluster_count,
        std::uint32_t max_cluster_node_count);

    rdg_texture* m_hzb{nullptr};
    rhi_sampler* m_hzb_sampler{nullptr};
    vec4f m_frustum;

    rdg_buffer* m_draw_buffer{nullptr};
    rdg_buffer* m_draw_count_buffer{nullptr};
    rdg_buffer* m_draw_info_buffer{nullptr};

    rdg_buffer* m_cluster_queue{nullptr};
    rdg_buffer* m_cluster_queue_state{nullptr};
};
} // namespace violet