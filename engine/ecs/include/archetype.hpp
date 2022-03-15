#pragma once

#include "ecs_exports.hpp"
#include "storage.hpp"
#include <functional>
#include <tuple>
#include <unordered_map>

namespace ash::ecs
{
class ECS_API archetype_layout
{
public:
    struct layout_info
    {
        const component_info* data;
        std::size_t offset;
    };

    using info_list = std::vector<std::pair<component_id, layout_info>>;

public:
    archetype_layout(std::size_t capacity = storage::CHUNK_SIZE);

    void insert(const component_set& components)
    {
        for (auto [id, info] : components)
            m_layout[id] = {info, 0};

        rebuild();
    }

    void erase(const component_set& components)
    {
        for (auto [id, info] : components)
        {
            auto iter = m_layout.find(id);
            if (iter != m_layout.end())
                m_layout.erase(iter);
        }

        rebuild();
    }

    std::size_t get_entity_per_chunk() const { return m_entity_per_chunk; }

    auto begin() { return m_layout.begin(); }
    auto end() { return m_layout.end(); }
    auto begin() const { return m_layout.begin(); }
    auto end() const { return m_layout.end(); }

    auto cbegin() const { return m_layout.cbegin(); }
    auto cend() const { return m_layout.cend(); }

    auto find(component_id type) { return m_layout.find(type); }

    layout_info& at(component_id type) { return m_layout.at(type); }
    layout_info& operator[](component_id type) { return at(type); }

private:
    void rebuild();

    std::size_t m_capacity;
    std::size_t m_entity_per_chunk;
    std::unordered_map<component_id, layout_info> m_layout;
};

template <typename... Components>
class archetype_handle
{
public:
    using self_type = archetype_handle<Components...>;

public:
    archetype_handle(std::size_t index, storage* s, archetype_layout* layout)
        : m_index(index),
          m_storage(s),
          m_layout(layout)
    {
        m_offset = {m_layout->at(component_trait<Components>::id).offset...};
    }

    template <typename Component>
    Component& get_component()
    {
        auto index = std::div(
            static_cast<const long>(m_index),
            static_cast<const long>(m_layout->get_entity_per_chunk()));

        uint8_t* data = m_storage->get_chunk(index.quot)->data();
        std::size_t offset = m_offset[component_list<Components...>::template index<Component>()] +
                             sizeof(Component) * index.rem;

        return *reinterpret_cast<Component*>(data + offset);
    }

    self_type operator+(std::size_t offset)
    {
        return self_type(m_index + offset, m_storage, m_layout);
    }

    self_type operator-(std::size_t offset)
    {
        return self_type(m_index - offset, m_storage, m_layout);
    }

private:
    std::size_t m_index;
    storage* m_storage;
    archetype_layout* m_layout;

    std::array<std::size_t, sizeof...(Components)> m_offset;
};

class ECS_API archetype
{
    template <typename... Components>
    friend class archetype_handle;

public:
    template <typename... Components>
    using handle = archetype_handle<Components...>;

public:
    archetype(const archetype_layout& layout);

    void add();
    void remove(std::size_t index);
    void move(std::size_t index, archetype& target);

    inline std::size_t size() const noexcept { return m_storage.size(); }
    const archetype_layout& get_layout() const noexcept { return m_layout; }

    template <typename... Components>
    handle<Components...> begin()
    {
        return handle<Components...>(0, &m_storage, &m_layout);
    }

    template <typename... Components>
    handle<Components...> end()
    {
        return handle<Components...>(size(), &m_storage, &m_layout);
    }

private:
    void construct(storage::handle where);
    void move_construct(storage::handle source, storage::handle target);
    void destruct(storage::handle where);
    void swap(storage::handle a, storage::handle b);

    void* get_component(std::size_t index, component_id type);

    archetype_layout m_layout;
    storage m_storage;
};
} // namespace ash::ecs