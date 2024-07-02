#pragma once

#include "common/type_index.hpp"
#include "graphics/render_context.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class renderer
{
public:
    renderer();
    virtual ~renderer();

    virtual void render(
        render_graph& graph,
        const render_context& context,
        const render_camera& camera) = 0;
};
} // namespace violet