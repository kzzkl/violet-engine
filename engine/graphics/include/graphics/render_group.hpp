#pragma once

#include <cstdint>

namespace ash::graphics
{
enum render_groups : std::uint32_t
{
    RENDER_GROUP_1 = 1,
    RENDER_GROUP_2 = 1 << 1,
    RENDER_GROUP_3 = 1 << 2,
    RENDER_GROUP_4 = 1 << 3,
    RENDER_GROUP_5 = 1 << 4,
    RENDER_GROUP_6 = 1 << 5,
    RENDER_GROUP_7 = 1 << 6,
    RENDER_GROUP_UI = 1 << 7,
    RENDER_GROUP_DEBUG = 1 << 8,
    RENDER_GROUP_EDITOR = 1 << 9
};
} // namespace ash::graphics