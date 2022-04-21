#pragma once

#include "event.hpp"
#include "input.hpp"

namespace ash::window
{
struct event_keyboard_key
{
    using dispatcher = core::sequence_dispatcher<std::function<void(keyboard_key, key_state)>>;
};

struct event_mouse_move
{
    using dispatcher = core::sequence_dispatcher<std::function<void(int, int)>>;
};
} // namespace ash::window