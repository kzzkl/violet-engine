#include "graphics/render_graph/render_node.hpp"

namespace violet
{
render_node::render_node(graphics_context* context) : m_context(context)
{
}

render_node::~render_node()
{
}
} // namespace violet