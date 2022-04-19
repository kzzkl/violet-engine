#pragma once

#include "input.hpp"
#include <string_view>

namespace ash::window
{
struct window_rect
{
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t width;
    std::uint32_t height;
};

class window_impl
{
public:
    using mouse_type = mouse;
    using keyboard_type = keyboard;

public:
    virtual ~window_impl() = default;

    virtual bool initialize(std::uint32_t width, std::uint32_t height, std::string_view title) = 0;
    virtual void tick() = 0;
    virtual void show() = 0;

    virtual const void* handle() const = 0;
    virtual window_rect rect() const = 0;

    virtual mouse_type& mouse() = 0;
    virtual keyboard_type& keyboard() = 0;

    virtual void title(std::string_view title) = 0;
};
} // namespace ash::window