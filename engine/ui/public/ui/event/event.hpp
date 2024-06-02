#pragma once

#include "ui/rendering/ui_painter.hpp"
#include "window/input.hpp"
#include <functional>

namespace violet
{
enum widget_event_type : uint8_t
{
    WIDGET_EVENT_TYPE_PAINT,
    WIDGET_EVENT_TYPE_PAINT_END,
    WIDGET_EVENT_TYPE_LAYOUT_UPDATE,

    WIDGET_EVENT_TYPE_MOUSE_MOVE,
    WIDGET_EVENT_TYPE_MOUSE_ENTER,
    WIDGET_EVENT_TYPE_MOUSE_LEAVE,
    WIDGET_EVENT_TYPE_MOUSE_PRESS,
    WIDGET_EVENT_TYPE_MOUSE_RELEASE,
    WIDGET_EVENT_TYPE_MOUSE_WHEEL,
    WIDGET_EVENT_TYPE_MOUSE_CLICK,

    WIDGET_EVENT_TYPE_DRAG_START,
    WIDGET_EVENT_TYPE_DRAG,
    WIDGET_EVENT_TYPE_DRAG_END,

    WIDGET_EVENT_TYPE_BLUR,
    WIDGET_EVENT_TYPE_FOCUS,
    WIDGET_EVENT_TYPE_SHOW,
    WIDGET_EVENT_TYPE_HIDE,

    WIDGET_EVENT_TYPE_INPUT,
    WIDGET_EVENT_TYPE_RESIZE
};

class widget_event
{
public:
    static widget_event paint(ui_painter* painter);
    static widget_event paint_end(ui_painter* painter);
    static widget_event layout_update();

    static widget_event mouse_move(int x, int y);
    static widget_event mouse_enter();
    static widget_event mouse_leave();
    static widget_event mouse_press(mouse_key key, int x, int y);
    static widget_event mouse_release(mouse_key key, int x, int y);
    static widget_event mouse_wheel(int wheel);
    static widget_event mouse_click(mouse_key key, int x, int y);

    static widget_event drag_start(mouse_key key, int x, int y);
    static widget_event drag(mouse_key key, int x, int y);
    static widget_event drag_end(mouse_key key, int x, int y);

public:
    union {
        struct
        {
            ui_painter* painter;
        } paint;

        struct
        {
            mouse_key key;
            int x;
            int y;
            int wheel;
        } mouse_state;

        struct
        {
            char key;
        } input;

        struct
        {
            std::uint32_t width;
            std::uint32_t height;
        } resize;
    } data;

    widget_event_type type;
};
} // namespace violet