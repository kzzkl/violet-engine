#include "ui/controls/button.hpp"
#include "log.hpp"

namespace ash::ui
{
button::button(std::string_view text, const font& font, const button_style& style)
    : panel(style.default_color),
      m_default_color(style.default_color),
      m_highlight_color(style.highlight_color)
{
    on_mouse_over = [this]() { color(m_highlight_color); };
    on_mouse_out = [this]() { color(m_default_color); };

    m_label = std::make_unique<label>(text, font, style.text_color);
    m_label->link(this);

    justify_content(LAYOUT_JUSTIFY_CENTER);
    align_items(LAYOUT_ALIGN_CENTER);
}

icon_button::icon_button(std::uint32_t index, const font& font, const icon_button_style& style)
    : font_icon(index, font, style.icon_scale, style.default_color),
      m_default_color(style.default_color),
      m_highlight_color(style.highlight_color)
{
    on_mouse_over = [this]() { icon_color(m_highlight_color); };
    on_mouse_out = [this]() { icon_color(m_default_color); };
}
} // namespace ash::ui