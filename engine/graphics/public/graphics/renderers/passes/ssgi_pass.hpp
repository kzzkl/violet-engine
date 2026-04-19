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
        rdg_texture* scene_color;
        rdg_texture* motion_vector;
        rdg_texture* normal_buffer;
        rdg_texture* hzb;
        rdg_buffer* irradiance_sh;

        rdg_texture* indirect_diffuse;
        rdg_texture* history;
        bool history_valid;

        bool bilateral_denoise{true};
        float bilateral_blur_factor{10.0f};

        float thickness;
        std::uint32_t iteration_count;

        std::uint32_t frame;

        debug_mode debug_mode = DEBUG_MODE_NONE;
        rdg_texture* debug_output = nullptr;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    void add_ssgi_pass(render_graph& graph, const parameter& parameter);
    void add_temporal_denoise_pass(render_graph& graph, const parameter& parameter);
    void add_bilatral_denoise_pass(render_graph& graph, const parameter& parameter);
    void add_debug_pass(render_graph& graph, const parameter& parameter);

    rdg_texture* m_ssgi_buffer;
};
} // namespace violet