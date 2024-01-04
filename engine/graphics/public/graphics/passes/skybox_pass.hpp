#pragma once

#include "graphics/render_graph/render_pass.hpp"

namespace violet
{
class skybox_pass : public render_pass
{
public:
    skybox_pass(renderer* renderer, setup_context& context);

    virtual void execute(execute_context& context) override;

private:
    render_resource* m_render_target;
    render_resource* m_depth_buffer;
    render_pipeline* m_pipeline;
};
} // namespace violet