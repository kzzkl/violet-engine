#pragma once

#include "ui/controls/panel.hpp"

namespace violet::ui
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
    void put_char(char c);

    virtual bool set_value(std::string_view text) { return true; }
    virtual std::string adjust_text() const noexcept { return ""; }

    std::unique_ptr<text_input> m_text;
    std::unique_ptr<panel> m_select;
    std::unique_ptr<panel> m_underline;

    bool m_text_change;

    std::uint32_t m_background_color;
    std::uint32_t m_underline_color;
};

class input_float : public input
{
public:
    input_float(const input_theme& theme);

    void value(float value);
    float value() const noexcept { return m_value; }

public:
    using on_value_change_event = element_event<void(float)>;

    on_value_change_event::handle on_value_change;

private:
    virtual bool set_value(std::string_view text) override;
    virtual std::string adjust_text() const noexcept override;

    float m_value;
};
} // namespace violet::ui