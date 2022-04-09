#pragma once

#include "archetype.hpp"
#include "entity.hpp"

namespace ash::ecs
{
class archetype_wrapper;
struct archetype_change_item
{
    entity entity;

    archetype_wrapper* archetype;
    std::size_t index;
};

using archetype_change_list = std::vector<archetype_change_item>;

template <typename... Components>
class archetype_wrapper_handle : public archetype_handle<Components...>
{
public:
    using base_type = archetype_handle<Components...>;
    using entity_type = entity;

public:
    archetype_wrapper_handle() noexcept {}
    archetype_wrapper_handle(std::vector<entity>* entities, base_type&& raw) noexcept
        : base_type(std::move(raw)),
          m_entities(entities)
    {
    }

    entity_type entity() const { return m_entities->at(base_type::index()); }

private:
    std::vector<entity_type>* m_entities;
};

class archetype_wrapper : public archetype
{
public:
    using base_type = archetype;

    template <typename... Components>
    using handle = archetype_wrapper_handle<Components...>;

public:
    archetype_wrapper(const archetype_layout& layout, const component_mask& mask)
        : archetype(layout),
          m_mask(mask)
    {
    }

    archetype_change_list add_entity(entity entity)
    {
        archetype::add();
        m_entities.push_back(entity);
        return archetype_change_list{
            {entity, this, m_entities.size() - 1}
        };
    }

    archetype_change_list move_entity(std::size_t index, archetype_wrapper& target)
    {
        archetype::move(index, target);

        archetype_change_list change_list;

        change_list.push_back({m_entities[index], &target, target.m_entities.size()});
        target.m_entities.push_back(m_entities[index]);

        if (index != m_entities.size() - 1)
        {
            change_list.push_back({m_entities.back(), this, index});
            m_entities[index] = m_entities.back();
        }
        m_entities.pop_back();

        return change_list;
    }

    template <typename... Components>
    handle<Components...> begin()
    {
        return handle<Components...>(&m_entities, base_type::begin<Components...>());
    }

    template <typename... Components>
    handle<Components...> end()
    {
        return handle<Components...>(&m_entities, base_type::end<Components...>());
    }

    const component_mask& mask() const noexcept { return m_mask; }

private:
    component_mask m_mask;
    std::vector<entity> m_entities;
};
} // namespace ash::ecs