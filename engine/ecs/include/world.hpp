#pragma once

#include "archetype.hpp"
#include "entity.hpp"
#include "view.hpp"
#include <atomic>
#include <queue>

namespace ash::ecs
{
using component_index = std::size_t;

static constexpr std::size_t MAX_COMPONENT = 512;
using component_mask = std::bitset<MAX_COMPONENT>;

class mask_archetype;
struct entity_record
{
    mask_archetype* archetype;
    std::size_t index;
};

class mask_archetype : public archetype
{
public:
    mask_archetype(
        const archetype_layout& layout,
        const std::unordered_map<component_id, component_index>& index_map)
        : archetype(layout)
    {
        for (const auto& [id, info] : layout)
            m_mask.set(index_map.at(id), true);
    }

    void add(entity_record* record)
    {
        archetype::add();

        record->archetype = this;
        record->index = m_record.size();

        m_record.push_back(record);
    }

    void remove(std::size_t index)
    {
        archetype::remove(index);

        std::swap(m_record[index], m_record.back());
        m_record[index]->index = index;
        m_record.pop_back();
    }

    void move(std::size_t index, mask_archetype& target)
    {
        archetype::move(index, target);

        entity_record* target_record = m_record[index];
        target_record->archetype = &target;
        target_record->index = target.m_record.size();
        target.m_record.push_back(target_record);

        std::swap(m_record[index], m_record.back());
        m_record[index]->index = index;
        m_record.pop_back();
    }

    const component_mask& get_mask() const noexcept { return m_mask; }

private:
    component_mask m_mask;
    std::vector<entity_record*> m_record;
};

class world
{
private:
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
    world() noexcept {}

    template <typename Component>
    void register_component()
    {
        component_id id = component_trait<Component>::id;

        auto iter = m_component_index.find(id);
        if (iter != m_component_index.end())
            return;

        component_index index = m_component_index_generator.new_index();
        m_component_index[id] = index;

        auto info = std::make_unique<component_info>();
        info->size = sizeof(Component);
        info->align = alignof(Component);
        info->construct = [](void* target) { new (target) Component(); };
        info->move_construct = [](void* source, void* target) {
            new (target) Component(std::move(*static_cast<Component*>(source)));
        };
        info->destruct = [](void* target) { static_cast<Component*>(target)->~Component(); };
        info->swap = [](void* a, void* b) {
            std::swap(*static_cast<Component*>(a), *static_cast<Component*>(b));
        };
        if (m_component_info.size() <= index)
            m_component_info.resize(index + 1);
        m_component_info[index] = std::move(info);
    }

    entity create() { return m_entity_index_generator.new_index(); }

    void release(entity entity) {}

    template <typename... Components>
    void add(entity entity)
    {
        auto& record = m_entity_record[entity];
        if (record.archetype == nullptr)
        {
            mask_archetype* archetype = get_or_create_archetype<Components...>();
            archetype->add(&record);
        }
        else
        {
            auto iter = m_archetypes.find(record.archetype->get_mask() | get_mask<Components...>());
            if (iter == m_archetypes.cend())
            {
                archetype_layout layout = record.archetype->get_layout();
                layout.insert(get_component_set<Components...>());

                mask_archetype* target = create_archetype(layout);
                record.archetype->move(record.index, *target);
            }
            else
            {
                record.archetype->move(record.index, *iter->second);
            }
        }
    }

    template <typename... Components>
    void remove(entity entity)
    {
        auto& record = m_entity_record[entity];
        mask_archetype* source = record.archetype;

        component_mask mask = source->get_mask() ^ get_mask<Components...>();
        auto iter = m_archetypes.find(mask);
        if (iter == m_archetypes.cend())
        {
            archetype_layout layout = source->get_layout();
            layout.erase(get_component_set<Components...>());

            mask_archetype* target = create_archetype(layout);
            source->move(record.index, *target);
        }
        else
        {
            source->move(record.index, *iter->second);
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
    component_set get_component_set()
    {
        component_set result;
        component_list<Components...>::each([&result, this ]<typename T>() {
            component_index index = m_component_index[component_trait<T>::id];
            result.push_back(std::make_pair(component_trait<T>::id, m_component_info[index].get()));
        });
        return result;
    }

    template <typename... Components>
    component_mask get_mask()
    {
        component_mask result;
        (result.set(m_component_index[component_trait<Components>::id]), ...);
        return result;
    }

    template <typename... Components>
    mask_archetype* get_or_create_archetype()
    {
        component_mask mask = get_mask<Components...>();
        auto& result = m_archetypes[mask];

        if (result == nullptr)
            return create_archetype<Components...>();
        else
            return result.get();
    }

    template <typename... Components>
    mask_archetype* create_archetype()
    {
        archetype_layout layout;
        layout.insert(get_component_set<Components...>());
        return create_archetype(layout);
    }

    mask_archetype* create_archetype(const archetype_layout& layout)
    {
        auto result = std::make_unique<mask_archetype>(layout, m_component_index);
        return (m_archetypes[result->get_mask()] = std::move(result)).get();
    }

    std::unordered_map<entity, entity_record> m_entity_record;
    std::unordered_map<component_mask, std::unique_ptr<mask_archetype>> m_archetypes;

    std::unordered_map<component_id, component_index> m_component_index;
    std::vector<std::unique_ptr<component_info>> m_component_info;

    index_generator<entity> m_entity_index_generator;
    index_generator<component_index> m_component_index_generator;
};
} // namespace ash::ecs