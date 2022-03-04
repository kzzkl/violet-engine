#pragma once

#include "plugin_interface.hpp"

namespace ash::graphics::external
{
struct adapter_info
{
    char description[128];
};

class diagnotor
{
public:
    virtual int get_adapter_info(adapter_info* infos, int size) = 0;
};

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

class graphics_context
{
public:
    virtual ~graphics_context() = default;

    virtual bool initialize() = 0;

    virtual graphics_factory* get_factory() = 0;
    virtual diagnotor* get_diagnotor() = 0;
};

using make_context = graphics_context* (*)();
} // namespace ash::graphics::external