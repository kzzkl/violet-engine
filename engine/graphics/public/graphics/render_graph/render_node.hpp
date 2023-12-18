#pragma once

#include "graphics/render_interface.hpp"
#include "graphics/renderer.hpp"

namespace violet
{
struct compile_context
{
    renderer* renderer;
};

struct execute_context
{
    renderer* renderer;
    rhi_render_command* command;

    rhi_parameter* camera;
    rhi_parameter* light;
};

class render_node
{
public:
    render_node();
    render_node(const render_node&) = delete;
    virtual ~render_node();

    render_node& operator=(const renderer&) = delete;

    virtual bool compile(compile_context& context) = 0;
    virtual void execute(execute_context& context) = 0;
};
} // namespace violet