#include "relation.hpp"

namespace ash::core
{
relation::relation() : system_base("relation")
{
}

bool relation::initialize(const dictionary& config)
{
    auto& event = system<core::event>();
    event.register_event<event_link>();
    event.register_event<event_unlink>();

    auto& world = system<ecs::world>();
    world.register_component<link_type>();

    return true;
}

void relation::link(ecs::entity child_entity, ecs::entity parent_entity)
{
    auto& world = system<ecs::world>();

    auto& child = world.component<link_type>(child_entity);
    auto& parent = world.component<link_type>(parent_entity);

    // If there is already a parent node, remove from the parent node.
    if (child.parent != ecs::INVALID_ENTITY)
        unlink(child_entity, true);

    parent.children.push_back(child_entity);
    child.parent = parent_entity;

    system<core::event>().publish<event_link>(child_entity, child);
}

void relation::unlink(ecs::entity entity, bool before_link)
{
    auto& world = system<ecs::world>();

    auto& child = world.component<link_type>(entity);
    auto& parent = world.component<link_type>(child.parent);

    for (auto iter = parent.children.begin(); iter != parent.children.end(); ++iter)
    {
        if (*iter == entity)
        {
            std::swap(*iter, parent.children.back());
            parent.children.pop_back();
            break;
        }
    }

    child.parent = ecs::INVALID_ENTITY;

    system<core::event>().publish<event_unlink>(
        entity,
        child,
        before_link ? event_unlink_flag::BEFORE_LINK : event_unlink_flag::UNLINK);
}
} // namespace ash::core