#pragma once

#include "ecs_exports.hpp"
#include "entity.hpp"
#include "storage.hpp"
#include <functional>
#include <tuple>
#include <unordered_map>

namespace ash::ecs
{
class ECS_API archetype_layout
{
public:
    struct component_info
    {
        struct
        {
            std::function<void(void*)> construct;
            std::function<void(void*, void*)> moveConstruct;
            std::function<void(void*)> destruct;
            std::function<void(void*, void*)> swap;
        } functor;

        struct
        {
            std::size_t offset;
            std::size_t size;
            std::size_t align;
        } layout;
    };

    using info_list = std::vector<std::pair<component_index, component_info>>;

public:
    archetype_layout(std::size_t capacity);

    template <typename... Components>
    void insert()
    {
        info_list list = get_info_list();
        component_list<Components...>::each([&list]<typename T>() {
            component_info info = {};
            info.layout.size = sizeof(T);
            info.layout.align = alignof(T);
            info.functor.construct = [](void* target) { new (target) T(); };
            info.functor.moveConstruct = [](void* source, void* target) {
                new (target) T(std::move(*static_cast<T*>(source)));
            };
            info.functor.destruct = [](void* target) { static_cast<T*>(target)->~T(); };
            info.functor.swap = [](void* a, void* b) {
                std::swap(*static_cast<T*>(a), *static_cast<T*>(b));
            };

            list.emplace_back(std::make_pair(component_trait<T>::index(), info));
        });

        m_mask = m_mask | component_list<Components...>::get_mask();

        rebuild(list);
    }

    template <typename... Components>
    void erase()
    {
        info_list list = get_info_list();
        auto iter = list.begin();
        while (iter != list.end())
        {
            if (((iter->first == component_trait<Components>::index()) || ...))
                iter = list.erase(iter);
            else
                ++iter;
        }

        m_mask = m_mask ^ component_list<Components...>::get_mask();

        rebuild(list);
    }

    std::size_t get_entity_per_chunk() const { return m_entity_per_chunk; }
    component_mask get_mask() const { return m_mask; }

    auto begin() { return m_layout.begin(); }
    auto end() { return m_layout.end(); }

    auto cbegin() const { return m_layout.cbegin(); }
    auto cend() const { return m_layout.cend(); }

    auto& operator[](component_index type) { return m_layout.operator[](type); }

private:
    void rebuild(info_list& list);
    info_list get_info_list() const;

    component_mask m_mask;
    std::size_t m_capacity;
    std::size_t m_entity_per_chunk;
    std::unordered_map<component_index, component_info> m_layout;
};

class ECS_API redirector
{
public:
    void map(entity entity, std::size_t index);
    void unmap(entity entity);

    entity get_enitiy(std::size_t index) const;
    std::size_t get_index(entity entity) const;

    bool has_entity(entity entity) const;
    std::size_t size() const;

private:
    std::unordered_map<entity, std::size_t> m_redirector;
    std::vector<entity> m_entities;
};

class archetype;
struct ECS_API archetype_raw_handle
{
    archetype* owner;
    std::size_t index;
};

template <typename... Components>
class archetype_handle
{
public:
    using self_type = archetype_handle<Components...>;

public:
    archetype_handle() : m_raw{nullptr, 0} {}
    archetype_handle(archetype* owner, std::size_t index) : m_raw{owner, index} { load(); }
    archetype_handle(const archetype_raw_handle& raw) : m_raw(raw) { load(); }

    template <typename Component>
    Component& get_component()
    {
        return *std::get<Component*>(m_components);
    }

    entity get_entity() const;

    self_type operator+(std::size_t offset) const
    {
        self_type result = *this;
        result.step(offset);
        return result;
    }

    self_type& operator++()
    {
        step(1);
        return *this;
    }

    bool operator==(const self_type& other) const
    {
        return m_raw.index == other.m_raw.index && m_raw.owner == other.m_raw.owner;
    }

    bool operator!=(const self_type& other) const { return !operator==(other); }

    std::tuple<Components&...> operator*() { return {*std::get<Components*>(m_components)...}; }

private:
    void step(std::size_t offset);
    void load();

    archetype_raw_handle m_raw;
    std::tuple<Components*...> m_components;
};

class ECS_API archetype
{
    template <typename... Components>
    friend class archetype_handle;

public:
    using raw_handle = archetype_raw_handle;

    template <typename... Components>
    using handle = archetype_handle<Components...>;

public:
    archetype(const archetype_layout& layout);

    raw_handle add(entity entity);
    void remove(entity entity);
    void move(entity entity, archetype& target);

    bool has_entity(entity entity) const { return m_redirector.has_entity(entity); }
    entity get_entity(std::size_t index) const { return m_redirector.get_enitiy(index); }
    std::size_t size() const { return m_redirector.size(); }
    const archetype_layout& get_layout() const { return m_layout; }

    template <typename... Components>
    handle<Components...> begin()
    {
        return handle<Components...>(this, 0);
    }

    template <typename... Components>
    handle<Components...> end()
    {
        return handle<Components...>(this, size());
    }

    raw_handle find(entity entity) { return raw_handle{this, m_redirector.get_index(entity)}; }

private:
    void construct(storage::handle where);
    void move_construct(storage::handle source, storage::handle target);
    void destruct(storage::handle where);
    void swap(storage::handle a, storage::handle b);

    void* get_component(std::size_t index, component_index type);

    template <typename Component>
    Component* get_component(std::size_t index)
    {
        return static_cast<Component*>(get_component(index, component_trait<Component>::index()));
    }

    redirector m_redirector;
    archetype_layout m_layout;
    storage m_storage;
};

template <typename... Components>
entity archetype_handle<Components...>::get_entity() const
{
    return m_raw.owner->get_entity(m_raw.index);
}

template <typename... Components>
void archetype_handle<Components...>::load()
{
    m_components = std::make_tuple(m_raw.owner->get_component<Components>(m_raw.index)...);
}

template <typename... Components>
void archetype_handle<Components...>::step(std::size_t offset)
{
    std::size_t entity_per_chunk = m_raw.owner->m_storage.get_entity_per_chunk();
    std::size_t target = m_raw.index + offset;
    if (m_raw.index / entity_per_chunk == target / entity_per_chunk)
        std::apply([offset](auto&... com) { ((com += offset), ...); }, m_components);
    else
        m_components = std::make_tuple(m_raw.owner->get_component<Components>(target)...);

    m_raw.index = target;
}
} // namespace ash::ecs