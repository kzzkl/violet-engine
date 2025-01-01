#pragma once

#include "ui/event/event.hpp"

namespace violet
{
widget_event widget_event::paint(ui_painter* painter)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_PAINT;
    event.data.paint.painter = painter;
    return event;
}

widget_event widget_event::paint_end(ui_painter* painter)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_PAINT_END;
    event.data.paint.painter = painter;
    return event;
}

widget_event widget_event::layout_update()
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_LAYOUT_UPDATE;
    return event;
}

widget_event widget_event::mouse_move(int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_MOVE;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}

widget_event widget_event::mouse_enter()
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_ENTER;
    return event;
}

widget_event widget_event::mouse_leave()
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_LEAVE;
    return event;
}

widget_event widget_event::mouse_press(mouse_key key, int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_PRESS;
    event.data.mouse_state.key = key;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}

widget_event widget_event::mouse_release(mouse_key key, int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_RELEASE;
    event.data.mouse_state.key = key;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}

widget_event widget_event::mouse_wheel(int wheel)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_WHEEL;
    event.data.mouse_state.wheel = wheel;
    return event;
}

widget_event widget_event::mouse_click(mouse_key key, int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_MOUSE_CLICK;
    event.data.mouse_state.key = key;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}

widget_event widget_event::drag_start(mouse_key key, int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_DRAG_START;
    event.data.mouse_state.key = key;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}

widget_event widget_event::drag(mouse_key key, int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_DRAG;
    event.data.mouse_state.key = key;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}

widget_event widget_event::drag_end(mouse_key key, int x, int y)
{
    widget_event event = {};
    event.type = WIDGET_EVENT_TYPE_DRAG_END;
    event.data.mouse_state.key = key;
    event.data.mouse_state.x = x;
    event.data.mouse_state.y = y;
    return event;
}
} // namespace violet