#pragma once

#include "plugin_interface.hpp"

namespace ash::graphics::external
{
class graphics_factory
{
public:
    virtual ~graphics_factory() = default;
};

struct adapter_info
{
    char description[128];
};

class diagnotor
{
public:
    virtual int get_adapter_info(adapter_info* infos, int size) = 0;
};

class renderer
{
public:
    virtual ~renderer() = default;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
};

using window_handle = const void*;
struct graphics_context_config
{
    uint32_t width;
    uint32_t height;

    window_handle handle;

    bool msaa_4x;
};

class graphics_context
{
public:
    virtual ~graphics_context() = default;

    virtual bool initialize(const graphics_context_config& config) = 0;

    virtual graphics_factory* get_factory() = 0;
    virtual diagnotor* get_diagnotor() = 0;

    virtual renderer* get_renderer() = 0;
};

using make_context = graphics_context* (*)();
} // namespace ash::graphics::external