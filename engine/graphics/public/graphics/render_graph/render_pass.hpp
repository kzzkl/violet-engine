#pragma once

#include "graphics/render_graph/render_node.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/rhi.hpp"
#include <vector>

namespace violet
{
class render_pass : public render_node
{
public:
    render_pass(std::string_view name, std::size_t index);

    rhi_render_pass* get_interface();

private:
    // std::vector<std::vector<render_pipeline>> m_pipelines;
};
} // namespace violet