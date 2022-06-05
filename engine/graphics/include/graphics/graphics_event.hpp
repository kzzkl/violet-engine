#pragma once

#include "core/event.hpp"

namespace ash::graphics
{
struct event_render_extent_change
{
    using dispatcher = core::sequence_dispatcher<std::function<void(std::uint32_t, std::uint32_t)>>;
};
} // namespace ash::graphics