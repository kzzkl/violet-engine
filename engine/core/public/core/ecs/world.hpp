#pragma once

#include "core/ecs/entity.hpp"
#include "core/ecs/view.hpp"
#include <functional>
#include <queue>
#include <unordered_map>

namespace violet
{
struct entity_record
{
    std::size_t entity_index;
};

class world
{
public:
    template <typename... Components>
    using view_type = view<Components...>;

private:
    struct entity_info
    {
        std::uint16_t entity_version;
        std::uint16_t component_version;

        archetype* archetype;
        std::size_t archetype_index;
    };

public:
    world();
    ~world();

    [[nodiscard]] entity create(actor* owner);

    void release(entity entity);

    template <
        typename Component,
        typename ComponentInfo = component_info_default<Component>,
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
        entity_info& info = m_entity_infos[entity.index];

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
                components = old_archetype->get_components();
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
                old_archetype->move(info.archetype_index, *new_archetype);
            on_entity_move(entity.index, new_archetype, new_archetype_index);
        }
        else
        {
            std::size_t new_archetype_index = new_archetype->add();
            on_entity_move(entity.index, new_archetype, new_archetype_index);
        }
    }

    template <typename... Components>
    void remove_component(entity entity)
    {
        entity_info& info = m_entity_infos[entity.index];
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

                std::vector<component_id> old_components = info.archetype->get_components();
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
                old_archetype->move(info.archetype_index, *new_archetype);
            on_entity_move(entity.index, new_archetype, new_archetype_index);
        }
        else
        {
            info.archetype->remove(info.archetype_index);
            on_entity_move(entity.index, nullptr, 0);
        }
    }

    /**
     * @brief Confirm whether the entity is still valid. The value of index, the version of entity
     * and the version of archetype will be checked.
     *
     * @param entity
     * @return std::pair<bool, bool> The first bool indicates whether the entity version is valid,
     * and the second bool indicates whether the component reference is valid.
     */
    [[nodiscard]] std::pair<bool, bool> is_valid(entity entity) const;

    /**
     * @brief Get the version of entity.
     *
     * @param entity
     * @return std::pair<std::uint16_t, std::uint16_t> First uint16_t is the version of entity, and
     * the second uint16_t is the version of component.
     */
    [[nodiscard]] std::pair<std::uint16_t, std::uint16_t> get_version(entity entity) const;

    template <typename Component>
    [[nodiscard]] Component& get_component(entity entity)
    {
        assert(has_component<Component>(entity));

        auto iter = m_entity_infos[entity.index].archetype->begin() +
                    m_entity_infos[entity.index].archetype_index;
        return iter.get_component<Component>();
    }

    template <typename Component>
    [[nodiscard]] bool has_component(entity entity)
    {
        auto id = component_index::value<Component>();
        return has_component(entity, id);
    }

    [[nodiscard]] bool has_component(entity entity, component_id component)
    {
        assert(is_valid(entity).first);
        return m_entity_infos[entity.index].archetype->get_mask().test(component);
    }

private:
    friend class view_base;

    void on_entity_move(
        std::size_t entity_index,
        archetype* new_archetype,
        std::size_t new_archetype_index);

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

    archetype* make_archetype(const std::vector<component_id>& components);

    std::queue<std::uint32_t> m_free_entity;

    std::uint32_t m_view_version;

    std::unique_ptr<archetype_chunk_allocator> m_archetype_chunk_allocator;
    std::unordered_map<component_mask, std::unique_ptr<archetype>> m_archetypes;

    component_table m_component_table;
    std::vector<entity_info> m_entity_infos;
};
} // namespace violet