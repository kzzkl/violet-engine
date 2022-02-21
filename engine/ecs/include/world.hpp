#pragma once

#include "archetype_manager.hpp"
#include "ecs_exports.hpp"
#include "entity.hpp"
#include "view.hpp"
#include <queue>

namespace ash::ecs
{
template <typename... Commponents>
using entity_handle = archetype::handle<Commponents...>;

class ECS_API world
{
public:
    world();

    template <typename... Components>
    entity_handle<Components...> create()
    {
        entity entity = allocate_entity();
        return m_archetype.insert_entity<Components...>(entity);
    }

    void release(entity entity) { m_free_entity.push(entity); }

    template <typename... Components>
    void insert(entity entity)
    {
        m_archetype.insert_component<Components...>(entity);
    }

    template <typename... Components>
    void erase(entity entity)
    {
        m_archetype.erase_component<Components...>(entity);
    }

    template <typename... Components>
    view<Components...> get_view()
    {
        return m_archetype.get_view<Components...>();
    }

    template <typename... Components>
    entity_handle<Components...> get_component(entity entity)
    {
        return m_archetype.get_component<Components...>(entity);
    }

private:
    entity allocate_entity();

    entity m_nextentity;
    std::queue<entity> m_free_entity;

    archetype_manager m_archetype;
};
} // namespace ash::ecs