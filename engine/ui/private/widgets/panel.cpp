#include "ui/widgets/panel.hpp"

namespace violet
{
panel::panel() noexcept : m_color(COLOR_WHITE)
{
}

void panel::on_paint(ui_painter* painter)
{
    widget_extent extent = get_extent();
    painter->set_position(extent.x, extent.y);
    painter->set_color(m_color);
    painter->draw_rect(extent.width, extent.height);
}
} // namespace violet