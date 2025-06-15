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
        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;
    };

    static output add(render_graph& graph, const parameter& parameter);

private:
    struct cull_data
    {
        rdg_texture* hzb;
        rhi_sampler* hzb_sampler;
        vec4f frustum;
        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;
        rdg_buffer* cluster_queue;
        rdg_buffer* cluster_queue_state;
    };

    static void prepare_cluster_queue(render_graph& graph, rdg_buffer* cluster_queue);

    static void prepare_cluster_cull(
        render_graph& graph,
        rdg_buffer* cluster_queue_state,
        rdg_buffer* dispatch_command_buffer,
        bool cull_cluster);

    static void add_prepare_pass(
        render_graph& graph,
        rdg_buffer* count_buffer,
        rdg_buffer* cluster_queue_state);

    static void add_instance_cull_pass(render_graph& graph, const cull_data& cull_data);

    static void add_cluster_cull_pass(
        render_graph& graph,
        const cull_data& cull_data,
        std::uint32_t max_clusters,
        std::uint32_t max_cluster_nodes);

    static void add_cluster_cull_pass_persistent(
        render_graph& graph,
        const cull_data& cull_data,
        std::uint32_t max_clusters,
        std::uint32_t max_cluster_nodes);
};
} // namespace violet