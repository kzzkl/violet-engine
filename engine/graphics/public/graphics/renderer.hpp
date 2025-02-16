#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class renderer
{
public:
    renderer() = default;
    virtual ~renderer() = default;

    virtual void render(render_graph& graph) = 0;
};
} // namespace violet