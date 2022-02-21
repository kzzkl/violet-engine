#pragma once

#include "archetype.hpp"
#include "view.hpp"

namespace ash::ecs
{
template <typename... Commponents>
using view = base_view<archetype, Commponents...>;

class ECS_API archetype_manager
{
public:
    archetype_manager() = default;
    archetype_manager(const archetype_manager&) = delete;
    archetype_manager& operator=(const archetype_manager&) = delete;

    template <typename... Components>
    archetype::handle<Components...> insert_entity(entity entity)
    {
        m_archetype_map.resize(entity + 1);

        auto iter = m_archetypes.find(component_list<Components...>::get_mask());
        if (iter == m_archetypes.cend())
        {
            archetype* archetype = create_archetype<Components...>();
            m_archetype_map[entity] = archetype;
            return archetype->add(entity);
        }
        else
        {
            m_archetype_map[entity] = iter->second.get();
            return iter->second->add(entity);
        }
    }

    template <typename... Components>
    void insert_component(entity entity)
    {
        archetype* source = m_archetype_map[entity];

        component_mask mask = source->get_layout().get_mask() | component_list<Components...>::get_mask();

        auto iter = m_archetypes.find(mask);
        if (iter == m_archetypes.cend())
        {
            archetype_layout layout = source->get_layout();
            layout.insert<Components...>();

            archetype* target = create_archetype(layout);
            m_archetype_map[entity] = target;
            source->move(entity, *target);
        }
        else
        {
            m_archetype_map[entity] = iter->second.get();
            source->move(entity, *iter->second);
        }
    }

    template <typename... Components>
    void erase_component(entity entity)
    {
        archetype* source = m_archetype_map[entity];

        component_mask mask = source->get_layout().get_mask() ^ component_list<Components...>::get_mask();
        auto iter = m_archetypes.find(mask);
        if (iter == m_archetypes.cend())
        {
            archetype_layout layout = source->get_layout();
            layout.erase<Components...>();

            archetype* target = create_archetype(layout);
            m_archetype_map[entity] = target;
            source->move(entity, *target);
        }
        else
        {
            m_archetype_map[entity] = iter->second.get();
            source->move(entity, *iter->second);
        }
    }

    template <typename... Components>
    archetype::handle<Components...> get_component(entity entity)
    {
        return m_archetype_map[entity]->find(entity);
    }

    template <typename... Components>
    view<Components...> get_view()
    {
        view<Components...> result;
        component_mask viewMask = component_list<Components...>::get_mask();

        for (auto& [mask, archetype] : m_archetypes)
        {
            if ((viewMask & mask) == viewMask)
                result.insert(archetype.get());
        }

        return result;
    }

private:
    template <typename... Components>
    archetype* create_archetype()
    {
        archetype_layout layout(storage::CHUNK_SIZE);
        layout.insert<Components...>();
        return create_archetype(layout);
    }

    archetype* create_archetype(const archetype_layout& layout);

    std::vector<archetype*> m_archetype_map;
    std::unordered_map<component_mask, std::unique_ptr<archetype>> m_archetypes;
};
} // namespace ash::ecs