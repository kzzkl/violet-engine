#pragma once

#include "graphics/renderer.hpp"
#include "math/box.hpp"
#include "sample/imgui_pass.hpp"

namespace violet
{
class sdf_renderer : public renderer
{
public:
    sdf_renderer();

    void set_sdf(rhi_texture* sdf, const box3f& bounds)
    {
        m_sdf = sdf;
        m_bounds = bounds;
    }

    void set_matrix(const mat4f& matrix)
    {
        m_matrix = matrix;
    }

    void set_bounds_enable(bool enable)
    {
        m_bounds_enable = enable;
    }

    void set_brick_enable(bool enable)
    {
        m_brick_enable = enable;
    }

private:
    void on_render(render_graph& graph) override;

    void add_bounds_pass(render_graph& graph);

    rhi_extent m_render_extent;

    rdg_texture* m_render_target{nullptr};
    rdg_texture* m_depth_buffer{nullptr};

    rhi_texture* m_sdf{nullptr};

    box3f m_bounds;
    mat4f m_matrix{1.0f};

    bool m_bounds_enable{false};
    bool m_brick_enable{false};

    imgui_pass m_imgui_pass;
};
} // namespace violet