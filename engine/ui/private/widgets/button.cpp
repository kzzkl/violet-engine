#include "ui/widgets/button.hpp"
#include "ui/widgets/font_icon.hpp"
#include "ui/widgets/label.hpp"

namespace violet
{
button::button()
{
}

void button::on_paint(ui_draw_list* draw_list)
{
    draw_list->draw_box(0, 0, 100, 100);
}

/*icon_button::icon_button(std::uint32_t index, const icon_button_theme& theme)
    : font_icon(
          index,
          font_icon_theme{
              .icon_font = theme.icon_font,
              .icon_color = theme.default_color,
              .icon_scale = theme.icon_scale}),
      m_default_color(theme.default_color),
      m_highlight_color(theme.highlight_color)
{
    event()->on_mouse_over = [this]() { icon_color(m_highlight_color); };
    event()->on_mouse_out = [this]() { icon_color(m_default_color); };
}*/
} // namespace violet