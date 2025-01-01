#pragma once

#include "ui/color.hpp"
#include "ui/widget.hpp"

namespace violet
{
class panel : public widget
{
public:
    panel() noexcept;

    void set_color(ui_color color) noexcept { m_color = color; }

private:
    virtual void on_paint(ui_painter* painter) override;

    ui_color m_color{ui_color::WHITE};
};
} // namespace violet