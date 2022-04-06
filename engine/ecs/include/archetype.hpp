#pragma once

#include "component.hpp"
#include "storage.hpp"
#include <algorithm>
#include <unordered_map>

namespace ash::ecs
{
class archetype_layout
{
public:
    struct layout_info
    {
        component_info* component;
        std::size_t offset;
    };

public:
    archetype_layout(std::size_t capacity = CHUNK_SIZE) noexcept : m_capacity(capacity) {}

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

    std::size_t entity_per_chunk() const noexcept { return m_entity_per_chunk; }

    auto begin() { return m_layout.begin(); }
    auto end() { return m_layout.end(); }
    auto begin() const { return m_layout.begin(); }
    auto end() const { return m_layout.end(); }

    auto cbegin() const { return m_layout.cbegin(); }
    auto cend() const { return m_layout.cend(); }

    auto find(component_id type) { return m_layout.find(type); }

    bool empty() const noexcept { return m_layout.empty(); }

    layout_info& at(component_id type) { return m_layout.at(type); }
    layout_info& operator[](component_id type) { return at(type); }

private:
    void rebuild()
    {
        m_entity_per_chunk = 0;

        if (m_layout.empty())
            return;

        std::vector<std::pair<component_id, layout_info>> list;
        for (auto& [type, info] : m_layout)
            list.push_back(std::make_pair(type, info));

        std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
            return a.second.component->align() == b.second.component->align()
                       ? a.first < b.first
                       : a.second.component->align() > b.second.component->align();
        });

        std::size_t entity_size = 0;
        for (const auto& [type, info] : list)
            entity_size += info.component->size();

        m_entity_per_chunk = m_capacity / entity_size;

        std::size_t offset = 0;
        for (auto& [type, info] : list)
        {
            info.offset = offset;
            m_layout[type] = info;

            offset += info.component->size() * m_entity_per_chunk;
        }
    }

    std::size_t m_capacity;
    std::size_t m_entity_per_chunk;
    std::unordered_map<component_id, layout_info> m_layout;
};

template <typename... Components>
class archetype_handle
{
public:
    using self_type = archetype_handle<Components...>;
    using reference = self_type&;

    using storage_type = storage;
    using layout_type = archetype_layout;

public:
    archetype_handle(std::size_t index, storage_type* storage, layout_type* layout)
        : m_index(index),
          m_storage(storage),
          m_layout(layout)
    {
        m_offset = {m_layout->at(component_id_v<Components>).offset...};
    }

    template <typename Component>
    Component& component()
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(m_index),
            static_cast<const long>(m_layout->entity_per_chunk()));

        std::size_t offset = m_offset[component_list<Components...>::template index<Component>()] +
                             sizeof(Component) * entity_index;

        return *reinterpret_cast<Component*>(m_storage->get(chunk_index, offset));
    }

    self_type operator+(std::size_t offset)
    {
        self_type result = *this;
        result += offset;
        return result;
    }

    self_type operator-(std::size_t offset)
    {
        self_type result = *this;
        result -= offset;
        return result;
    }

    reference operator+=(std::size_t offset)
    {
        m_index += offset;
        return *this;
    }

    reference operator-=(std::size_t offset)
    {
        m_index -= offset;
        return *this;
    }

    reference index(std::size_t index) noexcept
    {
        m_index = index;
        return *this;
    }

private:
    std::size_t m_index;
    storage_type* m_storage;
    layout_type* m_layout;

    std::array<std::size_t, sizeof...(Components)> m_offset;
};

class archetype
{
public:
    template <typename... Components>
    using handle = archetype_handle<Components...>;

    using storage_type = storage;
    using layout_type = archetype_layout;

public:
    archetype(const layout_type& layout) noexcept : m_layout(layout), m_size(0) {}
    virtual ~archetype() { clear(); }

    void add() { construct(allocate()); }

    void remove(std::size_t index)
    {
        std::size_t back_index = m_size - 1;

        if (index != back_index)
            swap(index, back_index);

        destruct(back_index);

        --m_size;
        if (m_size % m_layout.entity_per_chunk() == 0)
            m_storage.pop_chunk();
    }

    void move(std::size_t index, archetype& target)
    {
        auto [source_chunk_index, source_entity_index] = std::div(
            static_cast<const long>(index),
            static_cast<const long>(m_layout.entity_per_chunk()));

        auto& target_layout = target.m_layout;
        std::size_t target_index = target.allocate();
        auto [target_chunk_index, target_entity_index] = std::div(
            static_cast<const long>(target_index),
            static_cast<const long>(target_layout.entity_per_chunk()));

        for (auto& [type, info] : m_layout)
        {
            auto iter = target_layout.find(type);
            if (iter != target_layout.end())
            {
                std::size_t source_offset =
                    info.offset + source_entity_index * info.component->size();
                std::size_t target_offset =
                    iter->second.offset + target_entity_index * iter->second.component->size();
                info.component->move_construct(
                    m_storage.get(source_chunk_index, source_offset),
                    target.m_storage.get(target_chunk_index, target_offset));
            }
        }

        for (auto [type, info] : target_layout)
        {
            if (m_layout.find(type) == m_layout.end())
            {
                std::size_t offset = info.offset + target_entity_index * info.component->size();
                info.component->construct(target.m_storage.get(target_chunk_index, offset));
            }
        }

        remove(index);
    }

    void clear() noexcept
    {
        for (std::size_t i = 0; i < m_size; ++i)
            destruct(i);

        m_storage.clear();
        m_size = 0;
    }

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

    const layout_type& layout() const noexcept { return m_layout; }

    inline std::size_t size() const noexcept { return m_size; }

private:
    std::size_t allocate()
    {
        std::size_t index = m_size++;
        if (index >= capacity())
            m_storage.push_chunk();
        return index;
    }

    void construct(std::size_t index)
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(index),
            static_cast<const long>(m_layout.entity_per_chunk()));

        for (auto& [type, info] : m_layout)
        {
            std::size_t offset = info.offset + entity_index * info.component->size();
            info.component->construct(m_storage.get(chunk_index, offset));
        }
    }

    void destruct(std::size_t index)
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(index),
            static_cast<const long>(m_layout.entity_per_chunk()));

        for (auto& [type, info] : m_layout)
        {
            std::size_t offset = info.offset + entity_index * info.component->size();
            info.component->destruct(m_storage.get(chunk_index, offset));
        }
    }

    void swap(std::size_t a, std::size_t b)
    {
        auto [a_chunk_index, a_entity_index] = std::div(
            static_cast<const long>(a),
            static_cast<const long>(m_layout.entity_per_chunk()));

        auto [b_chunk_index, b_entity_index] = std::div(
            static_cast<const long>(b),
            static_cast<const long>(m_layout.entity_per_chunk()));

        for (auto& [type, info] : m_layout)
        {
            std::size_t a_offset = info.offset + a_entity_index * info.component->size();
            std::size_t b_offset = info.offset + b_entity_index * info.component->size();
            info.component->swap(
                m_storage.get(a_chunk_index, a_offset),
                m_storage.get(b_chunk_index, b_offset));
        }
    }

    std::size_t capacity() const noexcept { return m_layout.entity_per_chunk() * m_storage.size(); }

    layout_type m_layout;
    storage_type m_storage;

    std::size_t m_size;
};
} // namespace ash::ecs