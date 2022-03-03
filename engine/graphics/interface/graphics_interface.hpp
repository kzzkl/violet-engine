#pragma once

#include "plugin_interface.hpp"

namespace ash::graphics::external
{
using window_handle = void*;

class renderer
{
public:
    virtual ~renderer() = default;

    virtual bool initialize(window_handle handle) = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

private:
};

class graphics_factory
{
public:
    virtual ~graphics_factory() = default;

    virtual renderer* make_renderer() = 0;

private:
};
} // namespace ash::graphics::external