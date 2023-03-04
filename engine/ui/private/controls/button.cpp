#include "ui/controls/button.hpp"
#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"

namespace violet::ui
{
button::button(std::string_view text, const button_theme& theme)
    : panel(theme.default_color),
      m_default_color(theme.default_color),
      m_highlight_color(theme.highlight_color)
{
    event()->on_mouse_over = [this]() { color(m_highlight_color); };
    event()->on_mouse_out = [this]() { color(m_default_color); };

    label_theme label_theme = {};
    label_theme.text_color = theme.text_color;
    label_theme.text_font = theme.text_font;
    m_label = std::make_unique<label>(text, label_theme);
    add(m_label.get());

    layout()->set_justify_content(LAYOUT_JUSTIFY_CENTER);
    layout()->set_align_items(LAYOUT_ALIGN_CENTER);
}

icon_button::icon_button(std::uint32_t index, const icon_button_theme& theme)
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
}
} // namespace violet::ui