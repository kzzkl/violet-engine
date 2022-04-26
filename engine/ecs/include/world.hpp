#pragma once

#include "archetype.hpp"
#include "view.hpp"
#include <queue>
#include <unordered_map>

namespace ash::ecs
{
class world
{
public:
    world() { register_component<information>(); }
    ~world()
    {
        for (auto& [_, archetype] : m_archetypes)
            archetype->clear();
    }

    template <typename Component, typename Constructer, typename... Args>
    void register_component(Args&&... args)
    {
        register_component<Component>(new Constructer(std::forward<Args>(args)...));
    }

    template <typename Component>
    void register_component()
    {
        register_component<Component>(new default_component_constructer<Component>());
    }

    [[nodiscard]] entity create(std::string_view name = "unnamed")
    {
        entity result = INVALID_ENTITY;
        if (m_free_entity.empty())
        {
            result = m_entity_registry.add();
        }
        else
        {
            entity result = m_free_entity.front();
            m_free_entity.pop();
            result = m_entity_registry.update(result);
        }

        archetype* archetype = get_or_create_archetype<information>();
        archetype->add(result);
        component<information>(result).name = name;

        return result;
    }

    template <typename... Components>
    void add(entity entity)
    {
        ASH_ASSERT(m_entity_registry[entity].archetype != nullptr);

        auto archetype = m_entity_registry[entity].archetype;
        component_mask new_mask = archetype->mask() | make_mask<Components...>();

        ASH_ASSERT(new_mask != archetype->mask());

        auto iter = m_archetypes.find(new_mask);
        if (iter == m_archetypes.cend())
        {
            auto components = archetype->components();
            (components.push_back(component_index::value<Components>()), ...);

            auto target = make_archetype(components);
            archetype->move(entity, *target);
        }
        else
        {
            archetype->move(entity, *iter->second);
        }
    }

    template <typename... Components>
    void remove(entity entity)
    {
        auto archetype = m_entity_registry[entity].archetype;
        component_mask new_mask = archetype->mask() ^ make_mask<Components...>();

        ASH_ASSERT(new_mask != archetype->mask());

        if (!new_mask.none())
        {
            auto iter = m_archetypes.find(new_mask);
            if (iter == m_archetypes.cend())
            {
                auto components = archetype->components();
                (components.push_back(component_index::value<Components>()), ...);

                auto target = make_archetype(components);
                archetype->move(entity, *target);
            }
            else
            {
                archetype->move(entity, *iter->second);
            }
        }
        else
        {
            archetype->remove(entity);
            release_entity(entity);
        }
    }

    void release(entity entity)
    {
        auto archetype = m_entity_registry[entity].archetype;
        archetype->remove(entity);
        release_entity(entity);
    }

    template <typename Component>
    [[nodiscard]] Component& component(entity entity)
    {
        ASH_ASSERT(has_component<Component>(entity));

        auto iter = m_entity_registry[entity].archetype->begin() + m_entity_registry[entity].index;
        return iter.component<Component>();
    }

    template <typename Component>
    [[nodiscard]] bool has_component(entity entity)
    {
        auto id = component_index::value<Component>();
        return m_entity_registry[entity].archetype->mask().test(id);
    }

    [[nodiscard]] bool has_component(entity entity, component_id component)
    {
        return m_entity_registry[entity].archetype->mask().test(component);
    }

    [[nodiscard]] const std::vector<component_id>& owned_component(entity entity)
    {
        ASH_ASSERT(m_entity_registry[entity].archetype != nullptr);

        auto archetype = m_entity_registry[entity].archetype;
        return archetype->components();
    }

    template <typename... Components>
    view<Components...>* make_view()
    {
        component_mask m = make_mask<Components...>();
        auto v = std::make_unique<view<Components...>>(m);

        for (auto& [mask, archetype] : m_archetypes)
        {
            if ((m & mask) == m)
                v->add_archetype(archetype.get());
        }

        auto result = v.get();
        m_views.push_back(std::move(v));
        return result;
    }

private:
    template <typename Component>
    void register_component(component_constructer* constructer)
    {
        m_component_registry[component_index::value<Component>()] =
            component_info(sizeof(Component), alignof(Component), constructer);
    }

    template <typename... Components>
    component_mask make_mask()
    {
        component_mask result;
        (result.set(component_index::value<Components>()), ...);
        return result;
    }

    template <typename... Components>
    archetype* get_or_create_archetype()
    {
        component_mask mask = make_mask<Components...>();
        auto& result = m_archetypes[mask];

        if (result == nullptr)
            return make_archetype<Components...>();
        else
            return result.get();
    }

    template <typename... Components>
    archetype* make_archetype()
    {
        std::vector<component_id> components;
        (components.push_back(component_index::value<Components>()), ...);
        return make_archetype(components);
    }

    archetype* make_archetype(const std::vector<component_id>& components)
    {
        auto result =
            std::make_unique<archetype>(components, m_component_registry, m_entity_registry);

        for (auto& v : m_views)
        {
            if ((v->mask() & result->mask()) == v->mask())
                v->add_archetype(result.get());
        }

        return (m_archetypes[result->mask()] = std::move(result)).get();
    }

    void release_entity(entity entity)
    {
        auto& info = m_entity_registry[entity];
        info.archetype = nullptr;
        info.index = 0;
        ++info.version;

        m_free_entity.push(entity);
    }

    std::vector<std::unique_ptr<view_base>> m_views;

    std::queue<entity> m_free_entity;
    std::unordered_map<component_mask, std::unique_ptr<archetype>> m_archetypes;

    entity_registry m_entity_registry;
    component_registry m_component_registry;
};
} // namespace ash::ecs