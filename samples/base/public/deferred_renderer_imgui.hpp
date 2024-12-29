#pragma once

#include "graphics/renderers/deferred_renderer.hpp"

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
    void update_vertex();

    rhi_ptr<rhi_buffer> m_position;
    rhi_ptr<rhi_buffer> m_texcoord;
    rhi_ptr<rhi_buffer> m_color;
    rhi_ptr<rhi_buffer> m_index;

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

class deferred_renderer_imgui : public deferred_renderer
{
public:
    void render(render_graph& graph, const render_scene& scene, const render_camera& camera)
        override
    {
        deferred_renderer::render(graph, scene, camera);

        m_imgui_pass.add(
            graph,
            {
                .render_target = get_render_target(),
            });
    }

private:
    imgui_pass m_imgui_pass;
};
} // namespace violet