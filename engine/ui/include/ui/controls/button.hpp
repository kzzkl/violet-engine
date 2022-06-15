#pragma once

#include "ui/controls/font_icon.hpp"
#include "ui/controls/panel.hpp"

namespace ash::ui
{
class label;

struct button_theme
{
    const font* text_font;
    std::uint32_t text_color;
    std::uint32_t default_color;
    std::uint32_t highlight_color;
};

class button : public panel
{
public:
    button(std::string_view text, const button_theme& theme);

private:
    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;

    std::unique_ptr<label> m_label;
};

struct icon_button_theme
{
    const font* icon_font;
    float icon_scale;
    std::uint32_t default_color;
    std::uint32_t highlight_color;
};

class icon_button : public font_icon
{
public:
    icon_button(std::uint32_t index, const icon_button_theme& theme);

private:
    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;
};
} // namespace ash::ui