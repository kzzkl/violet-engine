#pragma once

#include "assert.hpp"
#include "component_handle.hpp"
#include "entity.hpp"
#include "entity_record.hpp"
#include "view.hpp"
#include <atomic>
#include <queue>

namespace ash::ecs
{
class world
{
public:
    using archetype_type = archetype_wrapper;

public:
    world() { register_component<all_entity>(); }
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

    entity create()
    {
        if (m_free_entity.empty())
        {
            return m_record.add();
        }
        else
        {
            entity result = m_free_entity.front();
            m_free_entity.pop();

            return result;
        }
    }

    void release(entity e)
    {
        ++e.version;
        m_free_entity.push(e);

        m_record.update(archetype_change_list{
            {e, nullptr, 0}
        });
    }

    template <typename... Components>
    void add(entity e)
    {
        if (m_record[e].archetype == nullptr)
        {
            archetype_type* archetype = get_or_create_archetype<all_entity, Components...>();
            m_record.update(archetype->add_entity(e));
        }
        else
        {
            auto& record = m_record[e];
            component_mask new_mask = record.archetype->mask() | make_mask<Components...>();
            if (new_mask == record.archetype->mask())
                return;

            auto iter = m_archetypes.find(new_mask);
            if (iter == m_archetypes.cend())
            {
                archetype_layout layout = record.archetype->layout();
                layout.insert(make_component_set<Components...>());

                archetype_type* target = make_archetype(layout);
                m_record.update(record.archetype->move_entity(record.index, *target));
            }
            else
            {
                m_record.update(record.archetype->move_entity(record.index, *iter->second));
            }
        }
    }

    template <typename... Components>
    void remove(entity e)
    {
        auto archetype = m_record[e].archetype;
        component_mask new_mask = archetype->mask() ^ make_mask<Components...>();
        if (new_mask == archetype->mask())
            return;

        auto iter = m_archetypes.find(new_mask);
        if (iter == m_archetypes.cend())
        {
            archetype_layout layout = archetype->layout();
            layout.erase(make_component_set<Components...>());

            archetype_type* target = make_archetype(layout);
            m_record.update(archetype->move_entity(m_record[e].index, *target));
        }
        else
        {
            m_record.update(archetype->move_entity(m_record[e].index, *iter->second));
        }
    }

    template <typename Component, template <typename T> class Handle = write>
    Handle<Component> component(entity e)
    {
        ASH_ASSERT(has_component<Component>(e));
        return Handle<Component>(e, &m_record);
    }

    template <typename Component>
    bool has_component(entity e)
    {
        auto component_index = m_component_index[component_id_v<Component>];
        return m_record[e].archetype->mask().test(component_index);
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

    bool vaild(entity e) const noexcept { return m_record.vaild(e); }

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

    template <typename Component>
    void register_component(component_constructer* constructer)
    {
        auto iter = m_component_index.find(component_id_v<Component>);
        if (iter != m_component_index.end())
            return;

        component_index index = m_component_index_generator.new_index();
        m_component_index[component_id_v<Component>] = index;

        auto info =
            std::make_unique<component_info>(sizeof(Component), alignof(Component), constructer);
        if (m_component_info.size() <= index)
            m_component_info.resize(index + 1);
        m_component_info[index] = std::move(info);
    }

    template <typename... Components>
    component_set make_component_set()
    {
        component_set result;
        type_list<Components...>::each([&result, this]<typename T>() {
            component_index index = m_component_index[component_id_v<T>];
            result.push_back(std::make_pair(component_id_v<T>, m_component_info[index].get()));
        });
        return result;
    }

    template <typename... Components>
    component_mask make_mask()
    {
        component_mask result;
        (result.set(m_component_index[component_id_v<Components>]), ...);
        return result;
    }

    component_mask make_mask(const archetype_layout& layout)
    {
        component_mask result;
        for (const auto& [id, _] : layout)
            result.set(m_component_index.at(id), true);
        return result;
    }

    template <typename... Components>
    archetype_type* get_or_create_archetype()
    {
        component_mask mask = make_mask<Components...>();
        auto& result = m_archetypes[mask];

        if (result == nullptr)
            return make_archetype<Components...>();
        else
            return result.get();
    }

    template <typename... Components>
    archetype_type* make_archetype()
    {
        archetype_layout layout;
        layout.insert(make_component_set<Components...>());
        return make_archetype(layout);
    }

    archetype_type* make_archetype(const archetype_layout& layout)
    {
        auto result = std::make_unique<archetype_type>(layout, make_mask(layout));

        for (auto& v : m_views)
        {
            if ((v->mask() & result->mask()) == v->mask())
                v->add_archetype(result.get());
        }

        return (m_archetypes[result->mask()] = std::move(result)).get();
    }

    entity_record m_record;
    std::queue<entity> m_free_entity;

    std::unordered_map<component_mask, std::unique_ptr<archetype_type>> m_archetypes;

    std::unordered_map<component_id, component_index> m_component_index;
    std::vector<std::unique_ptr<component_info>> m_component_info;

    std::vector<std::unique_ptr<view_base>> m_views;

    index_generator<component_index> m_component_index_generator;
};
} // namespace ash::ecs