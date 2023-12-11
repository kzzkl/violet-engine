#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet::sample
{
class pbr_render_graph : public render_graph
{
public:
    pbr_render_graph(renderer* renderer);
};
} // namespace violet::sample