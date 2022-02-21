#pragma once

#include "input.hpp"
#include <string_view>

namespace ash::window
{
class window_impl
{
public:
    virtual ~window_impl() = default;

    virtual bool initialize(uint32_t width, uint32_t height, std::string_view title) = 0;
    virtual void tick() = 0;
    virtual void show() = 0;
    virtual void* get_handle() = 0;

    virtual mouse& get_mouse() = 0;
};
} // namespace ash::window