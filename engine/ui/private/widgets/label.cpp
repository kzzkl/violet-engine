#include "ui/widgets/label.hpp"
#include "common/log.hpp"
#include "ui/font.hpp"
#include <cassert>

namespace violet
{
label::label() : m_font(nullptr), m_color(COLOR_WHITE)
{
}

void label::on_paint(ui_painter* painter)
{
    painter->set_position(get_layout()->get_x(), get_layout()->get_y());
    painter->set_color(m_color);
    painter->draw_text(m_text, m_font);
}

void label::on_layout_update()
{
    widget_extent extent = get_extent();
    log::debug("{} {} {} {}", extent.x, extent.y, extent.width, extent.height);
}
} // namespace violet