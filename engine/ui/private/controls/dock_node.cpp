#include "ui/controls/dock_node.hpp"
#include "ui/controls/dock_area.hpp"

namespace violet::ui
{
static std::size_t dock_counter = 0;

dock_node::dock_node(dock_area* area) : m_dock_area(area)
{
}

void dock_node::dock_width(float value) noexcept
{
    m_width = value;
    layout()->set_width_percent(value);
}

void dock_node::dock_height(float value) noexcept
{
    m_height = value;
    layout()->set_height_percent(value);
}
} // namespace violet::ui