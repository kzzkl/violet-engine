#pragma once

#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"

namespace ash::ui
{
struct button_style
{
    std::uint32_t text_color;
    std::uint32_t default_color;
    std::uint32_t highlight_color;
};

class button : public panel
{
public:
    static constexpr button_style default_style = {
        .text_color = COLOR_BLACK,
        .default_color = 0xFFFAFAFA,
        .highlight_color = 0xFFD8D8D9};

public:
    button(std::string_view text, const font& font, const button_style& style = default_style);

private:
    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;

    std::unique_ptr<label> m_label;
};

struct icon_button_style
{
    float icon_scale;
    std::uint32_t default_color;
    std::uint32_t highlight_color;
};

class icon_button : public font_icon
{
public:
    static constexpr icon_button_style default_style = {
        .icon_scale = 1.0f,
        .default_color = COLOR_BLACK,
        .highlight_color = 0xFFFAFAFA};

public:
    icon_button(
        std::uint32_t index,
        const font& font,
        const icon_button_style& style = default_style);

private:
    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;
};
} // namespace ash::ui