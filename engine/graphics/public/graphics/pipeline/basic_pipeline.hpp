#pragma once

#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class basic_pipeline : public render_pipeline
{
public:
    basic_pipeline(rhi_context* rhi);

private:
    virtual void render(rhi_render_command* command, render_context& context);
};
} // namespace violet