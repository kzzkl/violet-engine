#pragma once

#include "graphics/render_graph/pass.hpp"

namespace violet
{
class pass_batch
{
public:
    virtual ~pass_batch() {}

    virtual void execute(rhi_render_command* command, render_context* context) = 0;

private:
};

class render_pass_batch : public pass_batch
{
public:
    render_pass_batch(const std::vector<pass*> passes, renderer* renderer);

    virtual void execute(rhi_render_command* command, render_context* context) override;

private:
    rhi_ptr<rhi_render_pass> m_render_pass;
    std::vector<std::vector<render_pass*>> m_passes;
};
} // namespace violet