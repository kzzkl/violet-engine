#pragma once

#include "ui/widget.hpp"

namespace violet
{
class button : public widget
{
public:
    button();

    void set_font(font* font);
    void set_text(std::string_view text);
    void set_text_color(ui_color color) noexcept { m_text_color = color; }
    void set_background_color(ui_color color) noexcept { m_background_color = color; }

private:
    virtual void on_paint(ui_painter* painter) override;
    virtual bool on_mouse_press(mouse_key key, int x, int y) override;

    void measure_text();

    font* m_font;

    std::string m_text;
    ui_color m_text_color{ui_color::BLACK};
    ui_color m_background_color{ui_color::WHITE};
};
} // namespace violet