#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class skybox_pass : public rdg_render_pass
{
public:
    static constexpr std::size_t reference_render_target{0};
    static constexpr std::size_t reference_depth{1};

public:
    skybox_pass();

    virtual void execute(rhi_command* command, rdg_context* context) override;
};
} // namespace violet