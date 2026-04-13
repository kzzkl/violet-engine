#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class ssgi_pass
{
public:
    enum debug_mode
    {
        DEBUG_MODE_NONE,
        DEBUG_MODE_SSGI,
    };

    struct parameter
    {
        std::span<rdg_texture*> gbuffers;
        rdg_texture* render_target;
        rdg_texture* depth_buffer;

        debug_mode debug_mode = DEBUG_MODE_NONE;
        rdg_texture* debug_output = nullptr;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_hzb_pass(render_graph& graph, const parameter& parameter);
    void add_ssgi_pass(render_graph& graph, const parameter& parameter);
    void add_debug_pass(render_graph& graph, const parameter& parameter);

    rdg_texture* m_hzb{nullptr};
    rdg_texture* m_ssgi_radiance{nullptr};
};
} // namespace violet