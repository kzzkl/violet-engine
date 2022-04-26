#pragma once

#include "assert.hpp"
#include "component.hpp"
#include "render_pipeline.hpp"

namespace ash::graphics
{
struct visual
{
    enum mask_type : std::uint32_t
    {
        GROUP_1 = 1,
        GROUP_2 = GROUP_1 << 1,
        GROUP_3 = GROUP_2 << 1,
        GROUP_4 = GROUP_3 << 1,
        GROUP_5 = GROUP_4 << 1,
        GROUP_6 = GROUP_5 << 1,
        GROUP_7 = GROUP_6 << 1,
        UI = GROUP_7 << 1,
        DEBUG = UI << 1,
        EDITOR = DEBUG << 1
    };

    std::vector<render_unit> submesh;
    render_parameter* object;
    std::uint32_t mask{GROUP_1};
};
} // namespace ash::graphics