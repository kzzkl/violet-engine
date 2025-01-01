#include "ui/widget.hpp"
#include "layout/layout_yoga.hpp"

namespace violet
{
widget::widget() : m_state(0), m_visible(true), m_layer(0), m_parent(nullptr)
{
    m_layout = std::make_unique<widget_layout_yoga>();
}

widget::~widget()
{
}

widget_extent widget::get_extent() const
{
    return widget_extent{
        .x = m_layout->get_absolute_x(),
        .y = m_layout->get_absolute_y(),
        .width = m_layout->get_width(),
        .height = m_layout->get_height()};
}

bool widget::receive_event(const widget_event& event)
{
    switch (event.type)
    {
    case WIDGET_EVENT_TYPE_PAINT:
        on_paint(event.data.paint.painter);
        return true;
    case WIDGET_EVENT_TYPE_PAINT_END:
        on_paint_end(event.data.paint.painter);
        return true;
    case WIDGET_EVENT_TYPE_LAYOUT_UPDATE:
        on_layout_update();
        return false;
    case WIDGET_EVENT_TYPE_MOUSE_MOVE:
        return on_mouse_move(event.data.mouse_state.x, event.data.mouse_state.y);
    case WIDGET_EVENT_TYPE_MOUSE_ENTER:
        return on_mouse_enter();
    case WIDGET_EVENT_TYPE_MOUSE_LEAVE:
        return on_mouse_leave();
    case WIDGET_EVENT_TYPE_MOUSE_PRESS:
        return on_mouse_press(
            event.data.mouse_state.key,
            event.data.mouse_state.x,
            event.data.mouse_state.y);
    case WIDGET_EVENT_TYPE_MOUSE_RELEASE:
        return on_mouse_release(
            event.data.mouse_state.key,
            event.data.mouse_state.x,
            event.data.mouse_state.y);
    case WIDGET_EVENT_TYPE_MOUSE_WHEEL:
        return on_mouse_wheel(event.data.mouse_state.wheel);
    case WIDGET_EVENT_TYPE_MOUSE_CLICK:
        return on_mouse_click(
            event.data.mouse_state.key,
            event.data.mouse_state.x,
            event.data.mouse_state.y);
    case WIDGET_EVENT_TYPE_DRAG_START:
        return on_drag_start(
            event.data.mouse_state.key,
            event.data.mouse_state.x,
            event.data.mouse_state.y);
    case WIDGET_EVENT_TYPE_DRAG:
        return on_drag(
            event.data.mouse_state.key,
            event.data.mouse_state.x,
            event.data.mouse_state.y);
    case WIDGET_EVENT_TYPE_DRAG_END:
        return on_drag_end(
            event.data.mouse_state.key,
            event.data.mouse_state.x,
            event.data.mouse_state.y);
    default:
        return true;
    }
}
} // namespace violet