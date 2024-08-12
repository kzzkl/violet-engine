#pragma once

#include "ecs/entity.hpp"
#include "ecs/view.hpp"
#include <functional>
#include <queue>
#include <unordered_map>

namespace violet
{
class world
{
private:
    struct entity_info
    {
        std::uint32_t version;

        archetype* archetype;
        std::size_t archetype_index;
    };

public:
    world();
    ~world();

    [[nodiscard]] entity create();

    void destroy(entity entity);

    template <
        typename Component,
        typename ComponentInfo = component_constructor<Component>,
        typename... Args>
    void register_component(Args&&... args)
    {
        component_id id = component_index::value<Component>();
        assert(m_component_table[id] == nullptr);
        m_component_table[id] = std::make_unique<ComponentInfo>(std::forward<Args>(args)...);
    }

    template <typename Component>
    bool is_component_register()
    {
        component_id id = component_index::value<Component>();
        return m_component_table[id] != nullptr;
    }

    template <typename... Components>
    void add_component(entity entity)
    {
        assert(is_valid(entity) && "The entity is outdated.");
        (assert(
             m_component_table[component_index::value<Components>()] != nullptr &&
             "Component is not registered."),
         ...);

        entity_info& info = m_entity_infos[entity.id];

        archetype* old_archetype = info.archetype;
        archetype* new_archetype = nullptr;

        component_mask new_mask = make_mask<Components...>();
        if (old_archetype != nullptr)
            new_mask |= old_archetype->get_mask();

        auto iter = m_archetypes.find(new_mask);
        if (iter == m_archetypes.cend())
        {
            std::vector<component_id> components;
            if (old_archetype != nullptr)
                components = old_archetype->get_component_ids();
            (components.push_back(component_index::value<Components>()), ...);
            new_archetype = make_archetype(components);
        }
        else
        {
            new_archetype = iter->second.get();
        }

        if (old_archetype != nullptr)
        {
            std::size_t new_archetype_index =
                old_archetype->move(info.archetype_index, *new_archetype, m_world_version);
            on_entity_move(entity.id, new_archetype, new_archetype_index);
        }
        else
        {
            std::size_t new_archetype_index = new_archetype->add(m_world_version);
            on_entity_move(entity.id, new_archetype, new_archetype_index);
        }
    }

    template <typename... Components>
    void remove_component(entity entity)
    {
        assert(is_valid(entity) && "The entity is outdated.");
        (assert(
             m_component_table[component_index::value<Components>()] != nullptr &&
             "Component is not registered."),
         ...);

        entity_info& info = m_entity_infos[entity.id];
        assert(info.archetype);

        component_mask new_mask = info.archetype->get_mask() ^ make_mask<Components...>();
        assert(new_mask != info.archetype->get_mask());

        if (!new_mask.none())
        {
            archetype* old_archetype = info.archetype;
            archetype* new_archetype = nullptr;

            auto iter = m_archetypes.find(new_mask);
            if (iter == m_archetypes.cend())
            {
                component_mask remove_mask;
                (remove_mask.set(component_index::value<Components>()), ...);

                std::vector<component_id> old_components = info.archetype->get_component_ids();
                std::vector<component_id> new_components;
                for (component_id id : old_components)
                {
                    if (!remove_mask.test(id))
                        new_components.push_back(id);
                }

                new_archetype = make_archetype(new_components);
            }
            else
            {
                new_archetype = iter->second.get();
            }

            std::size_t new_archetype_index =
                old_archetype->move(info.archetype_index, *new_archetype, m_world_version);
            on_entity_move(entity.id, new_archetype, new_archetype_index);
        }
        else
        {
            info.archetype->remove(info.archetype_index);
            on_entity_move(entity.id, nullptr, 0);
        }
    }

    [[nodiscard]] bool is_valid(entity entity) const;

    template <typename Component>
    [[nodiscard]] Component& get_component(entity entity)
    {
        assert(has_component<Component>(entity));

        archetype* archetype = m_entity_infos[entity.id].archetype;

        auto [chunk_index, entity_offset] = std::div(
            static_cast<const long>(m_entity_infos[entity.id].archetype_index),
            static_cast<const long>(archetype->entity_per_chunk()));

        return *std::get<0>(
            archetype->get_components<Component>(chunk_index, entity_offset, m_world_version));
    }

    template <typename Component>
    [[nodiscard]] bool has_component(entity entity)
    {
        auto id = component_index::value<Component>();
        return has_component(entity, id);
    }

    [[nodiscard]] bool has_component(entity entity, component_id component)
    {
        assert(is_valid(entity));
        return m_entity_infos[entity.id].archetype->get_mask().test(component);
    }

    [[nodiscard]] std::uint32_t get_version() const noexcept
    {
        return m_world_version;
    }

    void add_version() noexcept
    {
        ++m_world_version;

        if (m_world_version == 0)
        {
            ++m_world_version;
        }
    }

    [[nodiscard]] view<> get_view(std::uint32_t system_version = 0) noexcept
    {
        return view(this, system_version);
    }

    [[nodiscard]] std::uint32_t get_view_version() const noexcept
    {
        return m_view_version;
    }

    [[nodiscard]] std::vector<archetype*> get_archetypes(
        const component_mask& include_mask, const component_mask& exclude_mask) const
    {
        std::vector<archetype*> result;

        for (auto& [mask, archetype] : m_archetypes)
        {
            if ((mask & exclude_mask).any())
            {
                continue;
            }

            if ((mask & include_mask) == include_mask)
            {
                result.push_back(archetype.get());
            }
        }

        return result;
    }

private:
    void on_entity_move(
        std::size_t entity_index, archetype* new_archetype, std::size_t new_archetype_index);

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
        {
            ++m_view_version;
            return make_archetype<Components...>();
        }
        else
        {
            return result.get();
        }
    }

    template <typename... Components>
    archetype* make_archetype()
    {
        std::vector<component_id> components;
        (components.push_back(component_index::value<Components>()), ...);
        return make_archetype(components);
    }

    archetype* make_archetype(std::span<const component_id> components);

    std::queue<std::uint32_t> m_free_entity;

    std::uint32_t m_view_version{1};
    std::uint32_t m_world_version{1};

    std::unique_ptr<archetype_chunk_allocator> m_archetype_chunk_allocator;
    std::unordered_map<component_mask, std::unique_ptr<archetype>> m_archetypes;

    component_table m_component_table;
    std::vector<entity_info> m_entity_infos;

    friend class view_base;
};
} // namespace violet