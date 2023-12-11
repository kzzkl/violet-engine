#pragma once

#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
class skybox_pipeline : public render_pipeline
{
public:
    skybox_pipeline(std::string_view name, renderer* renderer);

private:
    virtual void render(rhi_render_command* command, render_data& data) override;
};
} // namespace violet