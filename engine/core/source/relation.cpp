#include "core/relation.hpp"
#include "core/relation_event.hpp"

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
    ASH_ASSERT(child.parent == ecs::INVALID_ENTITY);

    auto& parent = world.component<link_type>(parent_entity);
    parent.children.push_back(child_entity);
    child.parent = parent_entity;

    system<event>().publish<event_link>(child_entity, child);
}

void relation::unlink(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& child = world.component<link_type>(entity);
    ASH_ASSERT(child.parent != ecs::INVALID_ENTITY);

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

    system<event>().publish<event_unlink>(entity, child);
}
} // namespace ash::core