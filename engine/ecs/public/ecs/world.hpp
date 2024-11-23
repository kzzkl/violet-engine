#pragma once

#include "ecs/entity.hpp"
#include "ecs/view.hpp"
#include "ecs/world_command.hpp"
#include <queue>
#include <thread>
#include <unordered_map>

namespace violet
{
class world
{
public:
    world();
    ~world();

    [[nodiscard]] entity create();

    void destroy(entity e);

    template <typename Component>
    void register_component(std::unique_ptr<component_builder>&& builder = nullptr)
    {
        assert(is_main_thread());
        assert(is_component_register<Component>() == false);

        component_id component_id = component_index::value<Component>();

        auto& component_info = m_components[component_id];
        component_info.builder = std::make_unique<component_builder_default<Component>>();

        if constexpr (is_companion_component<Component>)
        {
            using MainComponent = typename component_trait<Component>::main_component;

            assert(is_component_register<MainComponent>() == true);
            static_assert(!is_companion_component<MainComponent>);

            auto& main_component_info = m_components[component_index::value<MainComponent>()];
            main_component_info.add_companion_component(component_id);

            if (builder)
            {
                assert(builder->get_id() == component_id);
                component_info.builder = std::move(builder);
            }
            else
            {
                component_info.builder = std::make_unique<component_builder_default<Component>>();
            }
        }
    }

    template <typename Component>
    [[nodiscard]] bool is_component_register() const noexcept
    {
        component_id id = component_index::value<Component>();
        return m_components[id].builder != nullptr;
    }

    [[nodiscard]] component_builder* get_component_builder(component_id id) const
    {
        return m_components[id].builder.get();
    }

    template <typename... Components>
    void add_component(entity e)
        requires((!is_companion_component<Components> && ...))
    {
        assert(is_main_thread());
        assert(is_valid(e) && "The entity is outdated.");
        assert(is_component_register<Components>() && ...);

        entity_info& info = m_entities[e.id];

        archetype* old_archetype = info.archetype;
        archetype* new_archetype = nullptr;

        component_mask new_mask = get_mask<Components...>();
        if (old_archetype != nullptr)
        {
            new_mask |= old_archetype->get_mask();
        }

        auto iter = m_archetypes.find(new_mask);
        if (iter == m_archetypes.cend())
        {
            std::vector<component_id> components;
            if (old_archetype != nullptr)
            {
                components = old_archetype->get_component_ids();
            }
            get_ids<Components...>(components);

            new_archetype = create_archetype(components);
        }
        else
        {
            new_archetype = iter->second.get();
        }

        if (old_archetype == new_archetype)
        {
            return;
        }

        std::size_t new_archetype_index =
            old_archetype == nullptr ?
                new_archetype->add(m_world_version) :
                old_archetype->move(info.archetype_index, *new_archetype, m_world_version);

        move_entity(e.id, new_archetype, new_archetype_index);

        if constexpr ((std::is_same_v<Components, entity> || ...))
        {
            *std::get<0>(
                info.archetype->get_components<entity>(info.archetype_index, m_world_version)) = e;
        }
    }

    template <typename... Components>
    void remove_component(entity e)
        requires((!is_companion_component<Components> && ...))
    {
        assert(is_main_thread());
        assert(is_valid(e) && "The entity is outdated.");
        assert(is_component_register<Components>() && ...);

        entity_info& info = m_entities[e.id];

        component_mask remove_mask = get_mask<Components...>();

        component_mask new_mask = info.archetype->get_mask() & (~remove_mask);
        assert(new_mask != info.archetype->get_mask());

        archetype* old_archetype = info.archetype;
        archetype* new_archetype = nullptr;

        auto iter = m_archetypes.find(new_mask);
        if (iter == m_archetypes.cend())
        {
            std::vector<component_id> old_components = info.archetype->get_component_ids();
            std::vector<component_id> new_components;
            for (component_id id : old_components)
            {
                if (!remove_mask.test(id))
                {
                    new_components.push_back(id);
                }
            }

            new_archetype = create_archetype(new_components);
        }
        else
        {
            new_archetype = iter->second.get();
        }

        std::size_t new_archetype_index =
            old_archetype->move(info.archetype_index, *new_archetype, m_world_version);
        move_entity(e.id, new_archetype, new_archetype_index);
    }

    [[nodiscard]] bool is_valid(entity e) const;

    template <typename Component>
    [[nodiscard]] Component& get_component(entity e)
        requires(!std::is_same_v<Component, entity> || std::is_const_v<Component>)
    {
        assert(has_component<Component>(e));

        archetype* archetype = m_entities[e.id].archetype;
        std::size_t index = m_entities[e.id].archetype_index;
        return *std::get<0>(archetype->get_components<Component>(index, m_world_version));
    }

    template <typename Component>
    [[nodiscard]] bool has_component(entity e)
    {
        auto id = component_index::value<Component>();
        return has_component(e, id);
    }

    [[nodiscard]] bool has_component(entity e, component_id component)
    {
        assert(is_valid(e));
        return m_entities[e.id].archetype->get_mask().test(component);
    }

    template <typename... Components>
    [[nodiscard]] bool is_updated(entity e, std::uint32_t system_version) const
    {
        assert(is_valid(e));

        const entity_info& info = m_entities[e.id];
        std::size_t chunk_index = info.archetype_index / info.archetype->get_chunk_capacity();
        return info.archetype->is_updated<Components...>(chunk_index, system_version);
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

    [[nodiscard]] view<> get_view() noexcept
    {
        return view(this);
    }

    [[nodiscard]] std::uint32_t get_view_version() const noexcept
    {
        return m_view_version;
    }

    void execute(std::span<world_command*> commands);

    [[nodiscard]] std::vector<archetype*> get_archetypes(
        const component_mask& include_mask,
        const component_mask& exclude_mask) const
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

    void clear()
    {
        m_archetypes.clear();
        m_entities.clear();
    }

private:
    struct entity_info
    {
        std::uint32_t version;

        archetype* archetype;
        std::size_t archetype_index;
    };

    struct component_info
    {
        std::unique_ptr<component_builder> builder;

        component_mask companion_mask;
        std::vector<component_id> companion_components;

        void add_companion_component(component_id id)
        {
            companion_mask.set(id);
            companion_components.push_back(id);
        }
    };

    [[nodiscard]] bool is_main_thread() const noexcept
    {
        return m_main_thread_id == std::this_thread::get_id();
    }

    void destroy_entity(entity_id id);
    void move_entity(entity_id id, archetype* new_archetype, std::size_t new_archetype_index);

    template <typename... Components>
    [[nodiscard]] component_mask get_mask() const
    {
        component_mask result;
        (result.set(component_index::value<Components>()), ...);
        result |= (m_components[component_index::value<Components>()].companion_mask | ...);
        return result;
    }

    template <typename Component>
    void get_id(std::vector<component_id>& result) const
    {
        result.push_back(component_index::value<Component>());

        auto& companion_components =
            m_components[component_index::value<Component>()].companion_components;
        result.insert(result.end(), companion_components.begin(), companion_components.end());
    }

    template <typename... Components>
    [[nodiscard]] void get_ids(std::vector<component_id>& result) const
    {
        (get_id<Components>(result), ...);
    }

    archetype* create_archetype(std::span<const component_id> components);

    std::queue<std::uint32_t> m_free_entity;

    std::uint32_t m_view_version{1};
    std::uint32_t m_world_version{1};

    std::unique_ptr<archetype_chunk_allocator> m_archetype_chunk_allocator;
    std::unordered_map<component_mask, std::unique_ptr<archetype>> m_archetypes;

    std::array<component_info, MAX_COMPONENT_TYPE> m_components;
    std::vector<entity_info> m_entities;

    std::thread::id m_main_thread_id;

    friend class view_base;
};
} // namespace violet