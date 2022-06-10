#include "ui/controls/button.hpp"
#include "log.hpp"

namespace ash::ui
{
button::button(std::string_view text, const button_style& style)
    : panel(style.default_color),
      m_default_color(style.default_color),
      m_highlight_color(style.highlight_color)
{
    on_mouse_over = [this]() { color(m_highlight_color); };
    on_mouse_out = [this]() { color(m_default_color); };

    label_style label_style = {};
    label_style.text_color = style.text_color;
    label_style.text_font = style.text_font;
    m_label = std::make_unique<label>(text, label_style);
    m_label->link(this);

    justify_content(LAYOUT_JUSTIFY_CENTER);
    align_items(LAYOUT_ALIGN_CENTER);
}

icon_button::icon_button(std::uint32_t index, const icon_button_style& style)
    : font_icon(
          index,
          font_icon_style{
              .icon_font = style.icon_font,
              .icon_color = style.default_color,
              .icon_scale = style.icon_scale}),
      m_default_color(style.default_color),
      m_highlight_color(style.highlight_color)
{
    on_mouse_over = [this]() { icon_color(m_highlight_color); };
    on_mouse_out = [this]() { icon_color(m_default_color); };
}
} // namespace ash::ui