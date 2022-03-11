#pragma once

#include "input.hpp"
#include <string_view>

namespace ash::window
{
struct window_rect
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

class window_impl
{
public:
    virtual ~window_impl() = default;

    virtual bool initialize(uint32_t width, uint32_t height, std::string_view title) = 0;
    virtual void tick() = 0;
    virtual void show() = 0;

    virtual const void* get_handle() const = 0;
    virtual window_rect get_rect() const = 0;

    virtual mouse& get_mouse() = 0;

    virtual void set_title(std::string_view title) = 0;
};
} // namespace ash::window