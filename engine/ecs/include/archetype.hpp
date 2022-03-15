#pragma once

#include "component.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <tuple>
#include <unordered_map>

namespace ash::ecs
{
static constexpr std::size_t CHUNK_SIZE = 1024 * 16;

class archetype_layout
{
public:
    struct layout_info
    {
        const component_info* data;
        std::size_t offset;
    };

    using info_list = std::vector<std::pair<component_id, layout_info>>;

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

    std::size_t get_entity_per_chunk() const noexcept { return m_entity_per_chunk; }

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

        info_list list;
        for (auto& [type, info] : m_layout)
            list.push_back(std::make_pair(type, info));

        if (list.empty())
            return;

        std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
            return a.second.data->align == b.second.data->align
                       ? a.first < b.first
                       : a.second.data->align > b.second.data->align;
        });

        std::size_t entitySize = 0;
        for (const auto& [type, info] : list)
            entitySize += info.data->size;

        m_entity_per_chunk = m_capacity / entitySize;

        std::size_t offset = 0;
        for (auto& [type, info] : list)
        {
            info.offset = offset;
            m_layout[type] = info;

            offset += info.data->size * m_entity_per_chunk;
        }
    }

    std::size_t m_capacity;
    std::size_t m_entity_per_chunk;
    std::unordered_map<component_id, layout_info> m_layout;
};

class alignas(64) chunk
{
public:
    inline uint8_t* data() noexcept { return m_data.data(); }

private:
    std::array<uint8_t, CHUNK_SIZE> m_data;
};

struct archetype_storage
{
    std::unique_ptr<archetype_layout> layout;
    std::vector<std::unique_ptr<chunk>> chunks;
};

template <typename... Components>
class archetype_handle
{
public:
    using self_type = archetype_handle<Components...>;

public:
    archetype_handle(std::size_t index, archetype_storage* storage, archetype_layout* layout)
        : m_index(index),
          m_storage(storage),
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

        uint8_t* data = m_storage->chunks[index.quot]->data();
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
    archetype_storage* m_storage;
    archetype_layout* m_layout;

    std::array<std::size_t, sizeof...(Components)> m_offset;
};

class archetype
{
    template <typename... Components>
    friend class archetype_handle;

public:
    template <typename... Components>
    using handle = archetype_handle<Components...>;

public:
    archetype(const archetype_layout& layout) noexcept : m_layout(layout), m_size(0) {}

    void add() { construct(allocate()); }

    void remove(std::size_t index)
    {
        std::size_t back_index = m_size - 1;

        if (index != back_index)
            swap(index, back_index);

        destruct(back_index);

        --m_size;
        if (m_size % m_layout.get_entity_per_chunk() == 0)
            m_storage.chunks.pop_back();
    }

    void move(std::size_t index, archetype& target)
    {
        auto [source_chunk_index, source_entity_index] = std::div(
            static_cast<const long>(index),
            static_cast<const long>(m_layout.get_entity_per_chunk()));
        uint8_t* source_start = m_storage.chunks[source_chunk_index]->data();

        auto& target_layout = target.m_layout;
        std::size_t target_index = target.allocate();
        auto [target_chunk_index, target_entity_index] = std::div(
            static_cast<const long>(target_index),
            static_cast<const long>(target_layout.get_entity_per_chunk()));
        uint8_t* target_start = target.m_storage.chunks[target_chunk_index]->data();

        for (auto& [type, component] : m_layout)
        {
            auto iter = target_layout.find(type);
            if (iter != target_layout.end())
            {
                component.data->move_construct(
                    source_start + component.offset + source_entity_index * component.data->size,
                    target_start + iter->second.offset +
                        target_entity_index * iter->second.data->size);
            }
        }

        for (auto [type, component] : target_layout)
        {
            if (m_layout.find(type) == m_layout.end())
            {
                component.data->construct(
                    target_start + component.offset + target_entity_index * component.data->size);
            }
        }

        remove(index);
    }

    inline std::size_t size() const noexcept { return m_size; }
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
    std::size_t allocate()
    {
        std::size_t index = m_size++;
        if (index >= get_capacity())
            m_storage.chunks.push_back(std::make_unique<chunk>());
        return index;
    }

    void construct(std::size_t index)
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(index),
            static_cast<const long>(m_layout.get_entity_per_chunk()));
        uint8_t* chunk_start = m_storage.chunks[chunk_index]->data();

        for (auto& [type, component] : m_layout)
        {
            component.data->construct(
                chunk_start + component.offset + entity_index * component.data->size);
        }
    }

    void destruct(std::size_t index)
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(index),
            static_cast<const long>(m_layout.get_entity_per_chunk()));
        uint8_t* chunk_start = m_storage.chunks[chunk_index]->data();

        for (auto& [type, component] : m_layout)
        {
            component.data->destruct(
                chunk_start + component.offset + entity_index * component.data->size);
        }
    }

    void swap(std::size_t a, std::size_t b)
    {
        auto [a_chunk_index, a_entity_index] = std::div(
            static_cast<const long>(a),
            static_cast<const long>(m_layout.get_entity_per_chunk()));
        uint8_t* a_chunk_start = m_storage.chunks[a_chunk_index]->data();

        auto [b_chunk_index, b_entity_index] = std::div(
            static_cast<const long>(b),
            static_cast<const long>(m_layout.get_entity_per_chunk()));
        uint8_t* b_chunk_start = m_storage.chunks[b_chunk_index]->data();

        for (auto& [type, component] : m_layout)
        {
            component.data->swap(
                a_chunk_start + component.offset + a_entity_index * component.data->size,
                b_chunk_start + component.offset + b_entity_index * component.data->size);
        }
    }

    std::size_t get_capacity() const noexcept
    {
        return m_layout.get_entity_per_chunk() * m_storage.chunks.size();
    }

    archetype_layout m_layout;
    archetype_storage m_storage;

    std::size_t m_size;
};
} // namespace ash::ecs