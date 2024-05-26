#pragma once

#include "graphics/render_graph/render_graph.hpp"
#include "ui/control_mesh.hpp"

namespace violet
{
class ui_render_pass : public rdg_render_pass
{
public:
    ui_render_pass();

private:
};

class ui_render_graph : public render_graph
{
public:
    ui_render_graph();

    void set_mvp_matrix(const float4x4& mvp);
    void set_offset(const std::vector<float4>& offset);

private:
};
} // namespace violet