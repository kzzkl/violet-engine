#include "ui/widgets/dock_window.hpp"
#include "ui/widgets/dock_area.hpp"
#include "ui/widgets/font_icon.hpp"
#include "ui/widgets/label.hpp"
#include "ui/widgets/panel.hpp"
#include "ui/widgets/scroll_view.hpp"
#include "window/window_module.hpp"

namespace violet
{
dock_window::dock_window(std::string_view title, dock_area* area, const dock_window_theme& theme)
    : dock_node(area),
      m_drag_edge(LAYOUT_EDGE_ALL),
      m_drag_position(0)
{
    layout()->set_flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);

    m_tab = std::make_unique<panel>(theme.bar_color);
    m_tab->layout()->set_flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    m_tab->layout()->set_flex_wrap(LAYOUT_FLEX_WRAP_NOWRAP);
    m_tab->layout()->set_align_items(LAYOUT_ALIGN_CENTER);
    m_tab->layout()->set_padding(10.0f, LAYOUT_EDGE_HORIZONTAL);
    m_tab->layout()->set_padding(3.0f, LAYOUT_EDGE_VERTICAL);
    add(m_tab.get());
    m_tab->event()->on_mouse_drag_begin = [this](int x, int y)
    {
        if (m_dock_area != nullptr)
            m_dock_area->dock_begin(this, x, y);
    };
    m_tab->event()->on_mouse_drag = [this](int x, int y)
    {
        if (m_dock_area != nullptr)
            m_dock_area->dock_move(x, y);
    };
    m_tab->event()->on_mouse_drag_end = [this](int x, int y)
    {
        if (m_dock_area != nullptr)
            m_dock_area->dock_end(x, y);
    };

    label_theme title_theme = {};
    title_theme.text_font = theme.title_font;
    title_theme.text_color = theme.title_color;
    m_title = std::make_unique<label>(title, title_theme);
    m_tab->add(m_title.get());

    scroll_view_theme scroll_view_theme = {
        .scroll_speed = theme.scroll_speed,
        .bar_width = theme.bar_width,
        .bar_color = theme.bar_color,
        .slider_color = theme.slider_color,
        .background_color = theme.container_color};
    scroll_view_theme.background_color = theme.container_color;
    m_container = std::make_unique<scroll_view>(scroll_view_theme);
    m_container->layout()->set_flex_grow(1.0f);
    add(m_container.get());

    // auto& mouse = get_system<window_module>().mouse();
    m_container->event()->on_mouse_move = [&, this](int x, int y)
    {
        if (m_dock_area == nullptr)
            return;

        if (in_edge(m_dock_area, x, y) != LAYOUT_EDGE_ALL)
            return;

        switch (in_edge(this, x, y))
        {
        case LAYOUT_EDGE_LEFT:
        case LAYOUT_EDGE_RIGHT:
            // mouse.cursor(MOUSE_CURSOR_SIZE_WE);
            break;
        // case LAYOUT_EDGE_TOP:
        case LAYOUT_EDGE_BOTTOM:
            // mouse.cursor(MOUSE_CURSOR_SIZE_NS);
            break;
        default:
            break;
        }
    };

    m_container->event()->on_mouse_drag_begin = [this](int x, int y)
    {
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

    m_container->event()->on_mouse_drag = [this](int x, int y)
    {
        switch (m_drag_edge)
        {
        case LAYOUT_EDGE_LEFT:
            m_dock_area->resize(this, m_drag_edge, x - m_drag_position);
            m_drag_position = x;
            break;
        case LAYOUT_EDGE_TOP:
            m_dock_area->resize(this, m_drag_edge, y - m_drag_position);
            m_drag_position = y;
            break;
        case LAYOUT_EDGE_RIGHT:
            m_dock_area->resize(this, m_drag_edge, x - m_drag_position);
            m_drag_position = x;
            break;
        case LAYOUT_EDGE_BOTTOM:
            m_dock_area->resize(this, m_drag_edge, y - m_drag_position);
            m_drag_position = y;
            break;
        default:
            break;
        }
    };

    m_container->event()->on_mouse_drag_end = [this](int x, int y)
    {
        if (m_drag_edge != LAYOUT_EDGE_ALL)
        {
            m_dock_area->resize_end();
            m_drag_edge = LAYOUT_EDGE_ALL;
        }
    };

    m_container->event()->on_resize = [this](int width, int height)
    {
        if (!m_dock_area->resize())
        {
            if (on_window_resize)
                on_window_resize(width, height);
        }
    };
}

dock_window::dock_window(
    std::string_view title,
    std::uint32_t icon,
    dock_area* area,
    const dock_window_theme& theme)
    : dock_window(title, area, theme)
{
    font_icon_theme icon_theme = {};
    icon_theme.icon_font = theme.icon_font;
    icon_theme.icon_color = theme.icon_color;
    icon_theme.icon_scale = theme.icon_scale;
    m_icon = std::make_unique<font_icon>(icon, icon_theme);
    m_icon->layout()->set_margin(5.0f, LAYOUT_EDGE_RIGHT);
    m_tab->add(m_icon.get(), 0);
}

dock_window::~dock_window()
{
}

void dock_window::add_item(control* item)
{
    m_container->add_item(item);
}

void dock_window::remove_item(control* item)
{
    m_container->remove_item(item);
}

layout_edge dock_window::in_edge(control* control, int x, int y)
{
    auto& extent = control->extent();
    if (x - extent.x < 5.0f) // left
        return LAYOUT_EDGE_LEFT;
    else if (y - extent.y < 5.0f) // top
        return LAYOUT_EDGE_TOP;
    else if (extent.x + extent.width - x < 5.0f) // right
        return LAYOUT_EDGE_RIGHT;
    else if (extent.y + extent.height - y < 5.0f) // bottom
        return LAYOUT_EDGE_BOTTOM;
    else
        return LAYOUT_EDGE_ALL;
}
} // namespace violet