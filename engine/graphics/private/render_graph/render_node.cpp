#include "graphics/render_graph/render_node.hpp"

namespace violet
{
render_node::render_node(std::string_view name, graphics_context* context)
    : m_name(name),
      m_context(context)
{
}

render_node::~render_node()
{
}
} // namespace violet