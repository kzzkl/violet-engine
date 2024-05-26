#pragma once

#include "ui/widget.hpp"

namespace violet
{
class button : public widget
{
public:
    button();

private:
    virtual void on_paint(ui_draw_list* draw_list) override;
};
} // namespace violet