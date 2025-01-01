#pragma once

#include "ui/color.hpp"
#include "ui/widgets/dock_node.hpp"

namespace violet
{
class font;
class label;
class font_icon;
class panel;
class scroll_view;

struct dock_window_theme
{
    const font* icon_font;
    std::uint32_t icon_color;
    float icon_scale;

    const font* title_font;
    std::uint32_t title_color;

    float scroll_speed;
    float bar_width;
    std::uint32_t bar_color;
    std::uint32_t slider_color;
    std::uint32_t container_color;
};

class dock_window : public dock_node
{
public:
    dock_window(std::string_view title, dock_area* area, const dock_window_theme& theme);
    dock_window(
        std::string_view title,
        std::uint32_t icon,
        dock_area* area,
        const dock_window_theme& theme);
    virtual ~dock_window();

    void add_item(control* item);
    void remove_item(control* item);

public:
    using on_window_resize_event = control_event<void(int, int)>;

    on_window_resize_event::handle on_window_resize;

private:
    static layout_edge in_edge(control* control, int x, int y);

    std::unique_ptr<font_icon> m_icon;
    std::unique_ptr<label> m_title;
    std::unique_ptr<panel> m_tab;

    std::unique_ptr<scroll_view> m_container;

    layout_edge m_drag_edge;
    int m_drag_position;
};
} // namespace violet