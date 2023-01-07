#include "ui/controls/dock_element.hpp"
#include "ui/controls/dock_area.hpp"

namespace violet::ui
{
static std::size_t dock_counter = 0;

dock_element::dock_element(dock_area* area) : m_dock_area(area)
{
}

void dock_element::dock_width(float value) noexcept
{
    m_width = value;
    width_percent(value);
}

void dock_element::dock_height(float value) noexcept
{
    m_height = value;
    height_percent(value);
}
} // namespace violet::ui