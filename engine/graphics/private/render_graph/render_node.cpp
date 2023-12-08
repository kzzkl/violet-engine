#include "graphics/render_graph/render_node.hpp"

namespace violet
{
render_node::render_node(std::string_view name, renderer* renderer)
    : m_name(name),
      m_renderer(renderer)
{
}

render_node::~render_node()
{
}
} // namespace violet