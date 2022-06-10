#pragma once

#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"

namespace ash::ui
{
struct button_style
{
    const font* text_font{nullptr};
    std::uint32_t text_color{COLOR_BLACK};
    std::uint32_t default_color{0xFFFAFAFA};
    std::uint32_t highlight_color{0xFFD8D8D9};
};

class button : public panel
{
public:
    button(std::string_view text, const button_style& style);

private:
    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;

    std::unique_ptr<label> m_label;
};

struct icon_button_style
{
    const font* icon_font;
    float icon_scale{1.0f};
    std::uint32_t default_color{COLOR_BLACK};
    std::uint32_t highlight_color{0xFFFAFAFA};
};

class icon_button : public font_icon
{
public:
    icon_button(std::uint32_t index, const icon_button_style& style = {});

private:
    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;
};
} // namespace ash::ui