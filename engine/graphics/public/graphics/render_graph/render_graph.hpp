#pragma once

#include "graphics/render_graph/render_pass.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/render_graph/render_resource.hpp"
#include <memory>
#include <queue>

namespace violet
{
class render_graph
{
public:
    render_pass* add_render_pass();
    render_pipeline* add_pipeline();
    void add_resource();

    void remove(render_node* node);

    bool compile();
    void execute();

private:
};
} // namespace violet