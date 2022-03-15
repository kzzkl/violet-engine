#pragma once

#include "storage.hpp"
#include <algorithm>
#include <functional>
#include <tuple>
#include <unordered_map>

namespace ash::ecs
{
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
    archetype_layout(std::size_t capacity = storage::CHUNK_SIZE) noexcept : m_capacity(capacity) {}

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

class archetype
{
    template <typename... Components>
    friend class archetype_handle;

public:
    template <typename... Components>
    using handle = archetype_handle<Components...>;

public:
    archetype(const archetype_layout& layout) noexcept
        : m_layout(layout),
          m_storage(layout.get_entity_per_chunk())
    {
    }

    void add()
    {
        storage::handle handle = m_storage.push_back();
        construct(handle);
    }

    void remove(std::size_t index)
    {
        auto handle = m_storage.begin() + index;
        auto back = m_storage.end() - 1;

        if (handle != back)
            swap(handle, back);

        destruct(back);

        m_storage.pop_back();
    }

    void move(std::size_t index, archetype& target)
    {
        auto target_handle = target.m_storage.push_back();
        auto source_handle = m_storage.begin() + index;

        auto& target_layout = target.m_layout;
        auto& source_layout = m_layout;

        for (auto& [type, component] : source_layout)
        {
            if (target_layout.find(type) != target_layout.end())
            {
                component.data->move_construct(
                    source_handle.get_component(component.offset, component.data->size),
                    target_handle.get_component(component.offset, component.data->size));
            }
        }

        for (auto [type, component] : target_layout)
        {
            if (source_layout.find(type) == source_layout.end())
            {
                component.data->construct(
                    target_handle.get_component(component.offset, component.data->size));
            }
        }

        remove(index);
    }

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
    void construct(storage::handle where)
    {
        for (auto& [type, component] : m_layout)
            component.data->construct(where.get_component(component.offset, component.data->size));
    }

    void move_construct(storage::handle source, storage::handle target)
    {
        for (auto& [type, component] : m_layout)
        {
            component.data->move_construct(
                source.get_component(component.offset, component.data->size),
                target.get_component(component.offset, component.data->size));
        }
    }

    void destruct(storage::handle where)
    {
        for (auto& [type, component] : m_layout)
        {
            component.data->destruct(where.get_component(component.offset, component.data->size));
        }
    }

    void swap(storage::handle a, storage::handle b)
    {
        for (auto& [type, component] : m_layout)
        {
            component.data->swap(
                a.get_component(component.offset, component.data->size),
                b.get_component(component.offset, component.data->size));
        }
    }

    void* get_component(std::size_t index, component_id type)
    {
        auto& layout = m_layout[type];
        auto handle = m_storage.begin() + index;
        return handle.get_component(layout.offset, layout.data->size);
    }

    archetype_layout m_layout;
    storage m_storage;
};
} // namespace ash::ecs