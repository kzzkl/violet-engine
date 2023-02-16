#pragma once

#include <cstdint>

namespace violet::window
{
struct window_extent
{
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t width;
    std::uint32_t height;
};
} // namespace violet::window