#include "ui/controls/dock_window.hpp"
#include "ui/controls/dock_area.hpp"
#include "window/window.hpp"

namespace ash::ui
{
dock_window::dock_window(std::string_view title, dock_area* area, const dock_window_style& style)
    : dock_element(area),
      m_drag_edge(LAYOUT_EDGE_ALL),
      m_drag_position(0)
{
    flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);

    m_tab = std::make_unique<panel>(style.bar_color);
    m_tab->link(this);
    m_tab->on_mouse_drag_begin = [this](int x, int y) {
        if (m_dock_area != nullptr)
            m_dock_area->dock_begin(this, x, y);
    };
    m_tab->on_mouse_drag = [this](int x, int y) {
        if (m_dock_area != nullptr)
            m_dock_area->dock_move(x, y);
    };
    m_tab->on_mouse_drag_end = [this](int x, int y) {
        if (m_dock_area != nullptr)
            m_dock_area->dock_end(x, y);
    };

    label_style title_style = {};
    title_style.text_font = style.title_font;
    title_style.text_color = style.title_color;
    m_title = std::make_unique<label>(title, title_style);
    m_title->link(m_tab.get());

    m_container = std::make_unique<panel>(style.container_color);
    // m_container->flex_grow(1.0f);
    m_container->link(this);

    auto& mouse = system<window::window>().mouse();
    on_mouse_move = [&, this](int x, int y) {
        if (m_dock_area == nullptr)
            return;

        if (in_edge(m_dock_area, x, y) != LAYOUT_EDGE_ALL)
            return;

        switch (in_edge(this, x, y))
        {
        case LAYOUT_EDGE_LEFT:
        case LAYOUT_EDGE_RIGHT:
            mouse.cursor(window::MOUSE_CURSOR_SIZE_WE);
            break;
        // case LAYOUT_EDGE_TOP:
        case LAYOUT_EDGE_BOTTOM:
            mouse.cursor(window::MOUSE_CURSOR_SIZE_NS);
            break;
        default:
            break;
        }
    };

    on_mouse_drag_begin = [this](int x, int y) {
        if (in_edge(m_dock_area, x, y) != LAYOUT_EDGE_ALL)
            return;

        m_drag_edge = in_edge(this, x, y);
        switch (m_drag_edge)
        {
        case LAYOUT_EDGE_LEFT:
        case LAYOUT_EDGE_RIGHT:
            m_drag_position = x;
            break;
        case LAYOUT_EDGE_TOP:
        case LAYOUT_EDGE_BOTTOM:
            m_drag_position = y;
            break;
        default:
            break;
        }
    };

    on_mouse_drag = [this](int x, int y) {
        switch (m_drag_edge)
        {
        case LAYOUT_EDGE_LEFT:
            m_dock_area->dock_resize(this, m_drag_edge, x - m_drag_position);
            m_drag_position = x;
            break;
        case LAYOUT_EDGE_TOP:
            m_dock_area->dock_resize(this, m_drag_edge, y - m_drag_position);
            m_drag_position = y;
            break;
        case LAYOUT_EDGE_RIGHT:
            m_dock_area->dock_resize(this, m_drag_edge, x - m_drag_position);
            m_drag_position = x;
            break;
        case LAYOUT_EDGE_BOTTOM:
            m_dock_area->dock_resize(this, m_drag_edge, y - m_drag_position);
            m_drag_position = y;
            break;
        default:
            break;
        }
    };

    on_mouse_drag_end = [this](int x, int y) { m_drag_edge = LAYOUT_EDGE_ALL; };
}

layout_edge dock_window::in_edge(element* element, int x, int y)
{
    auto& extent = element->extent();
    if (x - extent.x < 10.0f) // left
        return LAYOUT_EDGE_LEFT;
    else if (y - extent.y < 10.0f) // top
        return LAYOUT_EDGE_TOP;
    else if (extent.x + extent.width - x < 10.0f) // right
        return LAYOUT_EDGE_RIGHT;
    else if (extent.y + extent.height - y < 10.0f) // bottom
        return LAYOUT_EDGE_BOTTOM;
    else
        return LAYOUT_EDGE_ALL;
}
} // namespace ash::ui