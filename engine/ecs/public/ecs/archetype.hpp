#pragma once

#include "ecs/component.hpp"
#include <array>
#include <cassert>
#include <vector>

namespace violet
{
class archetype_chunk;
class archetype_chunk_allocator;

using archetype_layout = std::vector<std::pair<component_id, component_builder*>>;

class archetype
{
public:
    archetype(const archetype_layout& layout, archetype_chunk_allocator* allocator) noexcept;

    virtual ~archetype();

    std::size_t add(std::uint32_t world_version);
    std::size_t move(std::size_t index, archetype& dst, std::uint32_t world_version);
    void remove(std::size_t index);
    void clear() noexcept;

    template <typename... Components>
    [[nodiscard]] std::tuple<Components*...> get_components(
        std::size_t index,
        std::uint32_t world_version) noexcept
    {
        auto [chunk_index, entity_index] =
            std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_capacity));

        static constexpr bool read_only = (std::is_const_v<Components> && ...);

        if constexpr (!read_only)
        {
            (set_version_if_needed<Components>(chunk_index, world_version), ...);
        }

        std::tuple<Components*...> components = {
            (static_cast<Components*>(get_data_pointer(
                 chunk_index,
                 get_component_info(component_index::value<Components>()).chunk_offset)) +
             entity_index)...,
        };
        return components;
    }

    [[nodiscard]] void* get_component_pointer(
        component_id component_id,
        std::size_t index,
        std::uint32_t world_version)
    {
        auto [chunk_index, entity_index] =
            std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_capacity));

        set_version(chunk_index, world_version, component_id);

        std::size_t offset = get_component_info(component_id).get_offset(entity_index);
        return get_data_pointer(chunk_index, offset);
    }

    template <typename... Components>
    [[nodiscard]] bool is_updated(std::size_t chunk_index, std::uint32_t system_version)
    {
        return (
            check_updated(chunk_index, system_version, component_index::value<Components>()) ||
            ...);
    }

    [[nodiscard]] std::vector<component_id> get_component_ids() const noexcept
    {
        std::vector<component_id> result;
        result.reserve(m_components.size());

        for (const auto& component : m_components)
        {
            result.push_back(component.id);
        }

        return result;
    }

    [[nodiscard]] std::size_t get_entity_count(std::size_t chunk_index) const noexcept
    {
        return chunk_index == m_chunks.size() - 1 ? ((m_entity_count - 1) % m_chunk_capacity) + 1 :
                                                    m_chunk_capacity;
    }

    [[nodiscard]] std::size_t get_entity_count() const noexcept
    {
        return m_entity_count;
    }

    [[nodiscard]] std::size_t get_chunk_count() const noexcept
    {
        return m_chunks.size();
    }

    [[nodiscard]] std::size_t get_chunk_capacity() const noexcept
    {
        return m_chunk_capacity;
    }

    [[nodiscard]] const component_mask& get_mask() const noexcept
    {
        return m_mask;
    }

private:
    struct component_info
    {
        component_id id;
        component_builder* builder;

        std::size_t chunk_offset;

        std::size_t get_offset(std::size_t entity_index) const
        {
            return chunk_offset + (entity_index * builder->get_size());
        }
    };

    std::size_t allocate();
    void move_construct(std::size_t src, std::size_t dst);
    void destruct(std::size_t index);
    void move_assignment(std::size_t src, std::size_t dst);

    const component_info& get_component_info(component_id component_id) const noexcept
    {
        return m_components[m_component_id_to_index[component_id]];
    }

    [[nodiscard]] std::size_t get_capacity() const noexcept
    {
        return m_chunk_capacity * m_chunks.size();
    }

    void* get_data_pointer(std::size_t chunk_index, std::size_t offset);

    void set_version(
        std::size_t chunk_index,
        std::uint32_t world_version,
        component_id component_id);
    std::uint32_t get_version(std::size_t chunk_index, component_id component_id);

    template <typename T>
    void set_version_if_needed(std::size_t chunk_index, std::uint32_t world_version)
    {
        if constexpr (!std::is_const_v<T>)
        {
            set_version(chunk_index, world_version, component_index::value<T>());
        }
    }

    bool check_updated(
        std::size_t chunk_index,
        std::uint32_t system_version,
        component_id component_id);

    std::vector<component_info> m_components;
    std::array<std::uint8_t, MAX_COMPONENT_TYPE> m_component_id_to_index;

    component_mask m_mask;

    std::size_t m_chunk_capacity{0};

    std::size_t m_entity_count{0};

    std::vector<archetype_chunk*> m_chunks;
    archetype_chunk_allocator* m_chunk_allocator{nullptr};
};
} // namespace violet