#pragma once

#include "core/event.hpp"
#include "core/link.hpp"
#include "ecs/entity.hpp"

namespace violet::core
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

} // namespace violet::core