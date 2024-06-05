#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class present_pass : public rdg_pass
{
public:
    static constexpr std::size_t reference_present_target{0};

public:
    present_pass()
    {
        add_texture(
            reference_present_target,
            RHI_ACCESS_FLAG_SHADER_READ | RHI_ACCESS_FLAG_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_PRESENT);
    }

    virtual void execute(rhi_command* command, rdg_context* context) override {}
};
} // namespace violet