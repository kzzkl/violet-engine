#pragma once

#include "common/assert.hpp"
#include "core/node/component.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace violet
{
class archetype_chunk;
class archetype_chunk_allocator;

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

    template <typename Component>
    Component& get_component()
    {
        auto [chunk_index, entity_index] = std::div(
            static_cast<const long>(m_offset),
            static_cast<const long>(m_archetype->m_entity_per_chunk));

        std::size_t id = component_index::value<Component>();
        std::size_t address = m_archetype->m_offset[id] +
                              entity_index * m_archetype->m_component_infos->at(id)->size();

        return *static_cast<Component*>(m_archetype->get_data_pointer(chunk_index, address));
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
        archetype_chunk_allocator* allocator) noexcept;

    virtual ~archetype();

    std::size_t add();
    std::size_t move(std::size_t index, archetype& target);
    void remove(std::size_t index);
    void clear() noexcept;

    [[nodiscard]] iterator begin() { return iterator(this, 0); }
    [[nodiscard]] iterator end() { return iterator(this, size()); }

    [[nodiscard]] inline std::size_t size() const noexcept { return m_size; }
    [[nodiscard]] inline std::size_t entity_per_chunk() const noexcept
    {
        return m_entity_per_chunk;
    }

    [[nodiscard]] inline const std::vector<component_id>& get_components() const noexcept
    {
        return m_components;
    }
    [[nodiscard]] inline const component_mask& get_mask() const noexcept { return m_mask; }

private:
    friend class iterator;

    void initialize_layout(const std::vector<component_id>& components);

    std::size_t allocate();
    void construct(std::size_t index);
    void destruct(std::size_t index);
    void swap(std::size_t a, std::size_t b);

    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return m_entity_per_chunk * m_chunks.size();
    }

    void* get_data_pointer(std::size_t chunk_index, std::size_t offset);

    const component_registry* m_component_infos;

    std::vector<component_id> m_components;
    component_mask m_mask;

    std::size_t m_size;

    std::size_t m_entity_per_chunk;
    archetype_chunk_allocator* m_chunk_allocator;
    std::vector<archetype_chunk*> m_chunks;

    std::array<std::uint16_t, MAX_COMPONENT> m_offset;
};
} // namespace violet