#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class default_renderer : public renderer
{
public:
    virtual void render(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera) override;

private:
    rdg_texture* m_render_target;
    rdg_texture* m_depth_buffer;
};
} // namespace violet