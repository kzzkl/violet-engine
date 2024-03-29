#pragma once

#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class basic_pipeline : public render_pipeline
{
public:
    basic_pipeline(std::string_view name, graphics_context* context);

private:
    virtual void render(rhi_render_command* command, render_data& data);
};
} // namespace violet