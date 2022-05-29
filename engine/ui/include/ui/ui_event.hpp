#pragma once

#include "core/event.hpp"

namespace ash::ui
{
struct event_calculate_layout
{
    using dispatcher = core::sequence_dispatcher<std::function<void()>>;
};
} // namespace ash::ui