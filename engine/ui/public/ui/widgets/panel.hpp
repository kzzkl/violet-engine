#pragma once

#include "ui/color.hpp"
#include "ui/widget.hpp"

namespace violet
{
class panel : public widget
{
public:
    panel() noexcept;

    void set_color(std::uint32_t color) noexcept { m_color = color; }

private:
    virtual void on_paint(ui_painter* painter) override;

    std::uint32_t m_color;
};
} // namespace violet