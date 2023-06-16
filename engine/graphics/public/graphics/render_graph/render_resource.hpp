#pragma once

#include "graphics/render_graph/render_node.hpp"

namespace violet
{
class render_resource : public render_node
{
public:
    render_resource(std::string_view name, rhi_context* rhi);
};
} // namespace violet