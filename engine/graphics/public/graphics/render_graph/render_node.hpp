#pragma once

#include "graphics/render_graph/render_context.hpp"
#include "graphics/rhi.hpp"

namespace violet
{
class render_node
{
public:
    render_node(render_context* context);
    render_node(const render_node&) = delete;
    virtual ~render_node();

    render_node& operator=(const render_context&) = delete;

    render_context* get_context() const noexcept { return m_context; }

private:
    render_context* m_context;
};
} // namespace violet