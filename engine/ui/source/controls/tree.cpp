#include "ui/controls/tree.hpp"
#include "window/window.hpp"

namespace ash::ui
{
tree_node::tree_node() : m_open(false)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    resize(100.0f, 0.0f, false, true, true, false);
}

void tree_node::tick()
{
}

tree::tree()
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
}
} // namespace ash::ui