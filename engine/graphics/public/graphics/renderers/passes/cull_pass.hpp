#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
enum cull_stage
{
    CULL_STAGE_MAIN_PASS,
    CULL_STAGE_POST_PASS,
};

class cluster_render_feature;
class cull_pass
{
public:
    struct parameter
    {
        cull_stage stage;

        rdg_texture* hzb;

        rdg_buffer* cluster_queue;
        rdg_buffer* cluster_queue_state;

        rdg_buffer* draw_buffer;
        rdg_buffer* draw_count_buffer;
        rdg_buffer* draw_info_buffer;

        rdg_buffer* recheck_instances;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void prepare_cluster_cull(
        render_graph& graph,
        rdg_buffer* dispatch_buffer,
        bool cull_cluster,
        bool recheck);

    void add_prepare_pass(render_graph& graph);

    void add_instance_cull_pass(render_graph& graph);
    void add_cluster_cull_pass(render_graph& graph);

    rdg_texture* m_hzb{nullptr};
    rhi_sampler* m_hzb_sampler{nullptr};
    vec4f m_frustum;

    rdg_buffer* m_draw_buffer{nullptr};
    rdg_buffer* m_draw_count_buffer{nullptr};
    rdg_buffer* m_draw_info_buffer{nullptr};

    rdg_buffer* m_cluster_queue{nullptr};
    rdg_buffer* m_cluster_queue_state{nullptr};

    rdg_buffer* m_recheck_instances{nullptr};

    cull_stage m_stage;
};
} // namespace violet