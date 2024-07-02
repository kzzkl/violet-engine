#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class present_pass : public rdg_pass
{
public:
    enum reference
    {
        REFERENCE_PRESENT
    };

public:
    present_pass()
    {
        add_texture(
            REFERENCE_PRESENT,
            RHI_ACCESS_FLAG_SHADER_READ | RHI_ACCESS_FLAG_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_PRESENT);
    }

    virtual void execute(rhi_command* command, rdg_context* context) override {}
};
} // namespace violet