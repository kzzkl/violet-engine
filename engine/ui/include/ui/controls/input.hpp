#pragma once

#include "ui/controls/panel.hpp"

namespace ash::ui
{
class font;
class text_input;

struct input_theme
{
    const font* text_font;
    std::uint32_t text_color;

    std::uint32_t background_color;
    std::uint32_t underline_color;
    std::uint32_t select_color;
};

class input : public panel
{
public:
    input(const input_theme& theme);

    void text(std::string_view content);
    std::string text() const noexcept;

public:
    using on_text_change_event = element_event<void(std::string_view)>;

    on_text_change_event::handle on_text_change;

private:
    std::unique_ptr<text_input> m_text;
    std::unique_ptr<panel> m_select;
    std::unique_ptr<panel> m_underline;

    bool m_text_change;

    std::uint32_t m_background_color;
    std::uint32_t m_underline_color;
};
} // namespace ash::ui