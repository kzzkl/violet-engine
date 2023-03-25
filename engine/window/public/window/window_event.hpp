#pragma once

#include "core/event/event.hpp"
#include "window/input.hpp"

namespace violet
{
struct event_mouse_move
{
    using dispatcher = sequence_dispatcher<std::function<void(mouse_mode, int, int)>>;
};

struct event_mouse_key
{
    using dispatcher = sequence_dispatcher<std::function<void(mouse_key, key_state)>>;
};

struct event_keyboard_key
{
    using dispatcher = sequence_dispatcher<std::function<void(keyboard_key, key_state)>>;
};

struct event_keyboard_char
{
    using dispatcher = sequence_dispatcher<std::function<void(char)>>;
};

struct event_window_resize
{
    using dispatcher = sequence_dispatcher<std::function<void(std::uint32_t, std::uint32_t)>>;
};

struct event_window_destroy
{
    using dispatcher = sequence_dispatcher<std::function<void()>>;
};
} // namespace violet