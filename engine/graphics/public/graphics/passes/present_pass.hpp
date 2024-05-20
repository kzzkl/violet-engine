#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class present_pass : public rdg_pass
{
public:
    present_pass()
    {
        add_texture(
            "target",
            RHI_ACCESS_FLAG_SHADER_READ | RHI_ACCESS_FLAG_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_PRESENT);
    }
};
} // namespace violet