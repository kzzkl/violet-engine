#include "core/event.hpp"
#include "ecs/entity.hpp"
#include <functional>

namespace violet
{
struct event_enter_scene
{
    using dispatcher = sequence_dispatcher<std::function<void(ecs::entity)>>;
};

struct event_exit_scene
{
    using dispatcher = sequence_dispatcher<std::function<void(ecs::entity)>>;
};
} // namespace violet