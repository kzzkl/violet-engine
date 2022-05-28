#include "core/event.hpp"
#include "ecs/entity.hpp"
#include <functional>

namespace ash::scene
{
struct event_enter_scene
{
    using dispatcher = core::sequence_dispatcher<std::function<void(ecs::entity)>>;
};

struct event_exit_scene
{
    using dispatcher = core::sequence_dispatcher<std::function<void(ecs::entity)>>;
};
} // namespace ash::scene