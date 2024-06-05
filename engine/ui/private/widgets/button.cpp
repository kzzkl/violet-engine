#include "ui/widgets/button.hpp"
#include "common/log.hpp"

namespace violet
{
button::button() : m_font(nullptr)
{
}

void button::set_text(std::string_view text)
{
    m_text = text;
    measure_text();
}

void button::set_font(font* font)
{
    m_font = font;
    measure_text();
}

void button::on_paint(ui_painter* painter)
{
    widget_extent extent = get_extent();

    painter->set_position(extent.x, extent.y);
    painter->set_color(m_background_color);
    painter->draw_rect(extent.width, extent.height);

    if (!m_text.empty())
    {
        painter->set_position(
            extent.x + (extent.width - painter->get_text_width(m_text, m_font)) / 2,
            extent.y + (extent.height - painter->get_text_height(m_text, m_font)) / 2);
        painter->set_color(m_text_color);
        painter->draw_text(m_text);
    }
}

bool button::on_mouse_press(mouse_key key, int x, int y)
{
    log::debug("{}: {} {}", key, x, y);
    return false;
}

void button::measure_text()
{
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