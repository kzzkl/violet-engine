#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class imgui_pass
{
public:
    struct draw_constant
    {
        mat4f mvp;
        std::uint32_t textures[64];
    };

    struct parameter
    {
        rdg_texture* render_target;
    };

    imgui_pass();

    void add(render_graph& graph, const parameter& parameter);

private:
    struct imgui_geometry
    {
        rhi_ptr<rhi_buffer> position;
        rhi_ptr<rhi_buffer> texcoord;
        rhi_ptr<rhi_buffer> color;
        rhi_ptr<rhi_buffer> index;
    };

    void update_vertex();

    std::vector<imgui_geometry> m_geometries;

    struct draw_call
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        rhi_scissor_rect scissor;
    };
    std::vector<draw_call> m_draw_calls;

    draw_constant m_constant;
};
} // namespace violet