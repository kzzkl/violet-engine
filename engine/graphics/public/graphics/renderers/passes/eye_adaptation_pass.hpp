#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class eye_adaptation_pass
{
public:
    struct parameter
    {
        rdg_texture* render_target;
        rdg_texture* exposure;

        float min_ev;
        float max_ev;

        float low_percent;
        float high_percent;
        float min_brightness;
        float max_brightness;
        float speed_down;
        float speed_up;

        float delta_time;

        rdg_texture* debug_output;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void prepare(render_graph& graph, const parameter& parameter);
    void histogram(render_graph& graph, const parameter& parameter);
    void eye_adaptation(render_graph& graph, const parameter& parameter);
    void apply_exposure(render_graph& graph, const parameter& parameter);

    void add_debug_pass(render_graph& graph, const parameter& parameter);

    rdg_texture* m_render_target{nullptr};
    rdg_buffer* m_histogram{nullptr};

    vec2f m_scale_offset;
};
} // namespace violet