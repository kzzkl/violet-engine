#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class skybox_pass : public rdg_render_pass
{
public:
    skybox_pass();

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    rdg_pass_reference* m_color;
};
} // namespace violet