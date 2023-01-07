#pragma once

#include "assert.hpp"
#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include "ecs/storage.hpp"
#include <algorithm>

namespace violet::ecs
{
template <typename Archetype>
class archetype_iterator
{
public:
    using archetype_type = Archetype;
    using self_type = archetype_iterator<Archetype>;

public:
    archetype_iterator(archetype_type* archetype) : archetype_iterator(archetype, 0) {}
    archetype_iterator(archetype_type* archetype, std::size_t offset)
        : m_archetype(archetype),
          m_offset(offset)
    {
    }

    entity entity()
    {
        return m_archetype->m_entity_registry->update(m_archetype->m_entities[m_offset]);
    }

    template <typename Component>
    Component& component()
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(m_offset),
            static_cast<const long>(m_archetype->m_entity_per_chunk));

        std::size_t id = component_index::value<Component>();
        std::size_t address = m_archetype->m_offset[id] +
                              entity_index * m_archetype->m_component_registry->at(id).size();

        return *static_cast<Component*>(m_archetype->m_storage.get(chunk_index, address));
    }

    self_type operator+(std::size_t offset) const noexcept
    {
        return self_type(m_archetype, m_offset + offset);
    }

    self_type& operator++() noexcept
    {
        ++m_offset;
        return *this;
    }

    self_type operator++(int) noexcept
    {
        self_type result(m_archetype, m_offset);
        ++m_offset;
        return result;
    }

    self_type& operator+=(std::size_t offset) noexcept
    {
        m_offset += offset;
        return *this;
    }

    bool operator==(const self_type& other) const noexcept
    {
        return m_archetype == other.m_archetype && m_offset == other.m_offset;
    }

    bool operator!=(const self_type& other) const noexcept { return !operator==(other); }

private:
    archetype_type* m_archetype;
    std::size_t m_offset;
};

class archetype
{
public:
    using iterator = archetype_iterator<archetype>;

public:
    archetype(
        const std::vector<component_id>& components,
        const component_registry& component_registry,
        entity_registry& entity_registry) noexcept
        : m_component_registry(&component_registry),
          m_entity_registry(&entity_registry),
          m_components(components)
    {
        for (component_id id : components)
            m_mask.set(id);
        initialize_layout(components);
    }

    virtual ~archetype() { clear(); }

    void add(entity entity)
    {
        std::size_t index = allocate(entity);
        construct(index);

        auto& info = m_entity_registry->at(entity);
        info.archetype = this;
        info.index = index;
    }

    void move(entity entity, archetype& target)
    {
        VIOLET_ASSERT(this != &target);

        auto& entity_info = m_entity_registry->at(entity);
        VIOLET_ASSERT(entity_info.archetype == this);

        auto [source_chunk_index, source_entity_index] = std::div(
            static_cast<const long>(entity_info.index),
            static_cast<const long>(m_entity_per_chunk));

        std::size_t target_index = target.allocate(entity);
        auto [target_chunk_index, target_entity_index] = std::div(
            static_cast<const long>(target_index),
            static_cast<const long>(target.m_entity_per_chunk));

        for (component_id id : m_components)
        {
            if (target.m_mask.test(id))
            {
                auto& info = m_component_registry->at(id);

                std::size_t source_offset = m_offset[id] + source_entity_index * info.size();
                std::size_t target_offset = target.m_offset[id] + target_entity_index * info.size();
                info.move_construct(
                    m_storage.get(source_chunk_index, source_offset),
                    target.m_storage.get(target_chunk_index, target_offset));
            }
        }

        for (component_id id : target.m_components)
        {
            if (!m_mask.test(id))
            {
                auto& info = m_component_registry->at(id);

                std::size_t offset = target.m_offset[id] + target_entity_index * info.size();
                info.construct(target.m_storage.get(target_chunk_index, offset));
            }
        }

        remove(entity_info.index);

        entity_info.archetype = &target;
        entity_info.index = target_index;
    }

    void remove(entity entity)
    {
        auto& entity_info = m_entity_registry->at(entity);
        VIOLET_ASSERT(entity_info.archetype == this);

        remove(entity_info.index);
    }

    void clear() noexcept
    {
        for (std::size_t i = 0; i < m_entities.size(); ++i)
            destruct(i);

        m_storage.clear();
        m_entities.clear();
    }

    [[nodiscard]] iterator begin() { return iterator(this, 0); }
    [[nodiscard]] iterator end() { return iterator(this, size()); }

    [[nodiscard]] inline std::size_t size() const noexcept { return m_entities.size(); }
    [[nodiscard]] inline std::size_t entity_per_chunk() const noexcept
    {
        return m_entity_per_chunk;
    }

    [[nodiscard]] inline const std::vector<component_id>& components() const noexcept
    {
        return m_components;
    }
    [[nodiscard]] inline const component_mask& mask() const noexcept { return m_mask; }

private:
    friend class iterator;

    void initialize_layout(const std::vector<component_id>& components)
    {
        m_entity_per_chunk = 0;

        struct layout_info
        {
            component_id id;
            std::size_t size;
            std::size_t align;
        };

        std::vector<layout_info> list(components.size());
        std::transform(
            components.cbegin(),
            components.cend(),
            list.begin(),
            [this](component_id id) {
                return layout_info{
                    id,
                    m_component_registry->at(id).size(),
                    m_component_registry->at(id).align()};
            });

        std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
            return a.align == b.align ? a.id < b.id : a.align > b.align;
        });

        std::size_t entity_size = 0;
        for (const auto& info : list)
            entity_size += info.size;

        m_entity_per_chunk = CHUNK_SIZE / entity_size;

        std::size_t offset = 0;
        for (const auto& info : list)
        {
            m_offset[info.id] = static_cast<std::uint16_t>(offset);
            offset += info.size * m_entity_per_chunk;
        }
    }

    void remove(std::size_t index)
    {
        std::size_t back_index = m_entities.size() - 1;

        if (index != back_index)
        {
            swap(index, back_index);
            m_entity_registry->at(m_entities[back_index]).index = index;
            std::swap(m_entities[index], m_entities[back_index]);
        }

        destruct(back_index);
        m_entities.pop_back();

        if (m_entities.size() % m_entity_per_chunk == 0)
            m_storage.pop_chunk();
    }

    std::size_t allocate(entity entity)
    {
        std::size_t index = m_entities.size();
        m_entities.push_back(entity);
        if (index >= capacity())
            m_storage.push_chunk();
        return index;
    }

    void construct(std::size_t index)
    {
        auto [chunk_index, entity_index] =
            std::div(static_cast<const long>(index), static_cast<const long>(m_entity_per_chunk));

        for (component_id id : m_components)
        {
            auto& info = m_component_registry->at(id);

            std::size_t offset = m_offset[id] + entity_index * info.size();
            info.construct(m_storage.get(chunk_index, offset));
        }
    }

    void destruct(std::size_t index)
    {
        auto [chunk_index, entity_index] =
            std::div(static_cast<const long>(index), static_cast<const long>(m_entity_per_chunk));

        for (component_id id : m_components)
        {
            auto& info = m_component_registry->at(id);

            std::size_t offset = m_offset[id] + entity_index * info.size();
            info.destruct(m_storage.get(chunk_index, offset));
        }
    }

    void swap(std::size_t a, std::size_t b)
    {
        auto [a_chunk_index, a_entity_index] =
            std::div(static_cast<const long>(a), static_cast<const long>(m_entity_per_chunk));

        auto [b_chunk_index, b_entity_index] =
            std::div(static_cast<const long>(b), static_cast<const long>(m_entity_per_chunk));

        for (component_id id : m_components)
        {
            auto& info = m_component_registry->at(id);

            std::size_t a_offset = m_offset[id] + a_entity_index * info.size();
            std::size_t b_offset = m_offset[id] + b_entity_index * info.size();
            info.swap(
                m_storage.get(a_chunk_index, a_offset),
                m_storage.get(b_chunk_index, b_offset));
        }
    }

    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return m_entity_per_chunk * m_storage.size();
    }

    storage m_storage;

    entity_registry* m_entity_registry;
    const component_registry* m_component_registry;

    std::vector<entity> m_entities;
    std::vector<component_id> m_components;
    component_mask m_mask;

    std::size_t m_entity_per_chunk;
    std::array<std::uint16_t, MAX_COMPONENT> m_offset;
};
} // namespace violet::ecs