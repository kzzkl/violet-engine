#pragma once

#include "ui/color.hpp"
#include "ui/widget.hpp"

namespace violet
{
class font;
class label : public widget
{
public:
    label();

    void set_text(std::string_view text) { m_text = text; }
    std::string get_text() const noexcept { return m_text; }

    void set_font(font* font) noexcept { m_font = font; }
    void set_color(std::uint32_t color) noexcept { m_color = color; }

private:
    virtual void on_paint(ui_painter* painter) override;
    virtual void on_layout_update() override;

    std::string m_text;
    font* m_font;
    std::uint32_t m_color;
};
} // namespace violet