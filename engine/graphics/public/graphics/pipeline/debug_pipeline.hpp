#pragma once

#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class debug_pipeline : public render_pipeline
{
public:
    debug_pipeline(graphics_context* context);

private:
    virtual void render(rhi_render_command* command, render_data& data);
};
} // namespace violet