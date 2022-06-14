#pragma once

#include "ui/controls/dock_element.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/controls/scroll_view.hpp"

namespace ash::ui
{
struct dock_window_style
{
    const font* icon_font{nullptr};
    std::uint32_t icon_color{COLOR_BLACK};
    float icon_scale{0.8f};

    const font* title_font{nullptr};
    std::uint32_t title_color{COLOR_BLACK};

    std::uint32_t bar_color{COLOR_WHITE};
    std::uint32_t container_color{COLOR_WHITE};
};

class dock_window : public dock_element
{
public:
    dock_window(std::string_view title, dock_area* area, const dock_window_style& style);

    void add(element* element);
    void remove(element* element);

public:
    using on_window_resize_event = element_event<void(int, int)>;

    on_window_resize_event::handle on_window_resize;

private:
    static layout_edge in_edge(element* element, int x, int y);

    std::unique_ptr<label> m_title;
    std::unique_ptr<panel> m_tab;

    std::unique_ptr<scroll_view> m_container;

    layout_edge m_drag_edge;
    int m_drag_position;
};
} // namespace ash::ui