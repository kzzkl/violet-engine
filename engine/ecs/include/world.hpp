#pragma once

#include "archetype.hpp"
#include "ecs_exports.hpp"
#include "entity.hpp"
#include "view.hpp"
#include <atomic>
#include <queue>

namespace ash::ecs
{
template <typename... Components>
using entity_handle = archetype::handle<Components...>;

struct entity_record
{
    archetype* archetype;
    std::size_t index;
};

class ECS_API world
{
public:
    template <typename T>
    struct index_generator
    {
    public:
        using index_type = T;

    public:
        index_generator(index_type base = 0) : m_next(base) {}
        index_type new_index() { return m_next.fetch_add(1); }

    private:
        std::atomic<index_type> m_next;
    };

public:
    world() noexcept;

    entity create() { return m_entity_index_generator.new_index(); }

    void release(entity entity) {}

    template <typename... Components>
    void add(entity entity)
    {
        auto& record = m_entity_record[entity];
        if (record.archetype == nullptr)
        {
            component_mask mask = component_list<Components...>::get_mask();

            archetype* archetype = get_or_create_archetype<Components...>();
            record.index = archetype->add();
            record.archetype = archetype;
        }
        else
        {
            archetype* source = record.archetype;

            component_mask mask =
                source->get_layout().get_mask() | component_list<Components...>::get_mask();

            auto iter = m_archetypes.find(mask);
            if (iter == m_archetypes.cend())
            {
                archetype_layout layout = source->get_layout();
                layout.insert<Components...>();

                archetype* target = create_archetype(layout);
                record.archetype = target;
                source->move(entity, *target);
            }
            else
            {
                record.archetype = iter->second.get();
                source->move(entity, *iter->second);
            }
        }
    }

    template <typename... Components>
    void remove(entity entity)
    {
        auto& record = m_entity_record[entity];
        archetype* source = record.archetype;

        component_mask mask =
            source->get_layout().get_mask() ^ component_list<Components...>::get_mask();
        auto iter = m_archetypes.find(mask);
        if (iter == m_archetypes.cend())
        {
            archetype_layout layout = source->get_layout();
            layout.erase<Components...>();

            archetype* target = create_archetype(layout);
            record.archetype = target;
            source->move(entity, *target);
        }
        else
        {
            record.archetype = iter->second.get();
            source->move(entity, *iter->second);
        }
    }

    template <typename T>
    T& get_component(entity entity)
    {
        auto& record = m_entity_record[entity];
        auto handle = record.archetype->begin<T>() + record.index;

        return handle.get_component<T>();
    }

private:
    template <typename... Components>
    archetype* get_or_create_archetype()
    {
        component_mask mask = component_list<Components...>::get_mask();
        auto& result = m_archetypes[mask];

        if (result == nullptr)
        {
            archetype_layout layout;
            layout.insert<Components...>();
            result = std::make_unique<archetype>(layout);
            return result.get();
        }
        else
        {
            return result.get();
        }
    }

    template <typename... Components>
    archetype* create_archetype()
    {
        archetype_layout layout;
        layout.insert<Components...>();
        return create_archetype(layout);
    }

    archetype* create_archetype(const archetype_layout& layout);

    index_generator<entity> m_entity_index_generator;

    std::unordered_map<entity, entity_record> m_entity_record;
    std::unordered_map<component_mask, std::unique_ptr<archetype>> m_archetypes;
};
} // namespace ash::ecs