#pragma once

#include "ecs/component.hpp"
#include "ecs/entity.hpp"
#include <algorithm>
#include <cassert>
#include <span>
#include <unordered_map>
#include <vector>

namespace violet
{
class archetype_chunk;
class archetype_chunk_allocator;

class archetype
{
public:
    archetype(
        std::span<const component_id> components,
        const component_table& component_table,
        archetype_chunk_allocator* allocator) noexcept;

    virtual ~archetype();

    std::size_t add(std::uint32_t world_version);
    std::size_t move(std::size_t index, archetype& target, std::uint32_t world_version);
    void remove(std::size_t index);
    void clear() noexcept;

    template <typename... Components>
    [[nodiscard]] std::tuple<Components*...> get_components(
        std::size_t chunk_index, std::size_t offset, std::uint32_t world_version) noexcept
    {
        static constexpr bool read_only = (std::is_const_v<Components> && ...);

        if constexpr (!read_only)
        {
            (set_version_if_needed<Components>(chunk_index, world_version), ...);
        }

        std::tuple<Components*...> components = {
            (static_cast<Components*>(get_data_pointer(
                 chunk_index,
                 m_component_infos[component_index::value<Components>()].chunk_offset)) +
             offset)...,
        };
        return components;
    }

    template <typename... Components>
    [[nodiscard]] bool is_updated(std::size_t chunk_index, std::uint32_t system_version)
    {
        return (
            check_updated(chunk_index, system_version, component_index::value<Components>()) ||
            ...);
    }

    [[nodiscard]] inline const std::vector<component_id>& get_component_ids() const noexcept
    {
        return m_components;
    }

    [[nodiscard]] inline std::size_t get_chunk_count() const noexcept
    {
        return m_chunks.size();
    }

    [[nodiscard]] inline std::size_t get_entity_count(std::size_t chunk_index) const noexcept
    {
        return chunk_index == m_chunks.size() - 1 ? m_size % m_entity_per_chunk :
                                                    m_entity_per_chunk;
    }

    [[nodiscard]] inline std::size_t get_entity_count() const noexcept
    {
        return m_size;
    }

    [[nodiscard]] inline std::size_t entity_per_chunk() const noexcept
    {
        return m_entity_per_chunk;
    }

    [[nodiscard]] inline const component_mask& get_mask() const noexcept
    {
        return m_mask;
    }

private:
    std::size_t allocate();
    void move_construct(std::size_t source, std::size_t target);
    void destruct(std::size_t index);

    [[nodiscard]] std::size_t capacity() const noexcept
    {
        return m_entity_per_chunk * m_chunks.size();
    }

    void* get_data_pointer(std::size_t chunk_index, std::size_t offset);

    void set_version(
        std::size_t chunk_index, std::uint32_t world_version, component_id component_id);
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
        std::size_t chunk_index, std::uint32_t system_version, component_id component_id);

    std::vector<component_id> m_components;

    struct component_info
    {
        std::size_t chunk_offset;
        std::size_t index;

        component_constructor_base* constructor;

        std::size_t get_offset(std::size_t entity_index) const noexcept
        {
            return chunk_offset + entity_index * constructor->get_size();
        }
    };
    std::array<component_info, MAX_COMPONENT> m_component_infos;

    component_mask m_mask;

    std::size_t m_size;

    std::size_t m_entity_per_chunk;
    archetype_chunk_allocator* m_chunk_allocator;
    std::vector<archetype_chunk*> m_chunks;
};
} // namespace violet