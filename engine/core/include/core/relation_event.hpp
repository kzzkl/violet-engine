#pragma once

#include "core/event.hpp"
#include "core/link.hpp"
#include "ecs/entity.hpp"

namespace ash::core
{
struct event_link
{
    using functor = std::function<void(ecs::entity, link&)>;
    using dispatcher = sequence_dispatcher<functor>;
};

struct event_unlink
{
    using functor = std::function<void(ecs::entity, link&)>;
    using dispatcher = sequence_dispatcher<functor>;
};

} // namespace ash::core