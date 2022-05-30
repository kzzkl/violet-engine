#pragma once

#include "assert.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ash::ecs
{
struct entity
{
    std::uint32_t index;
    std::uint32_t version;

    [[nodiscard]] bool operator==(const entity& other) const noexcept
    {
        return index == other.index && version == other.version;
    }

    [[nodiscard]] bool operator!=(const entity& other) const noexcept { return !operator==(other); }
    [[nodiscard]] bool operator<(const entity& other) const noexcept
    {
        if (index != other.index)
            return index < other.index;
        else
            return version < other.version;
    }
};

static constexpr entity INVALID_ENTITY = {0, 0};

struct information
{
    std::string name;
};

class archetype;
struct entity_info
{
    std::uint32_t version;

    archetype* archetype;
    std::size_t index;
};

class entity_registry
{
public:
    entity_registry()
    {
        // Index 0 means invalid entity.
        m_registry.push_back({.version = 1, .archetype = nullptr, .index = 0});
    }

    entity add()
    {
        entity result = {static_cast<std::uint32_t>(m_registry.size()), 0};
        m_registry.emplace_back();
        return result;
    }

    [[nodiscard]] entity update(entity entity) const noexcept
    {
        return {entity.index, m_registry[entity.index].version};
    }

    [[nodiscard]] bool vaild(entity entity) const noexcept
    {
        return entity.index < m_registry.size() &&
               m_registry[entity.index].version == entity.version;
    }

    [[nodiscard]] entity_info& at(entity entity) noexcept
    {
        ASH_ASSERT(vaild(entity));
        return m_registry.at(entity.index);
    }

    [[nodiscard]] const entity_info& at(entity entity) const noexcept
    {
        ASH_ASSERT(vaild(entity));
        return m_registry.at(entity.index);
    }

    [[nodiscard]] entity_info& operator[](entity entity) noexcept
    {
        ASH_ASSERT(vaild(entity));
        return m_registry[entity.index];
    }

    [[nodiscard]] const entity_info& operator[](entity entity) const noexcept
    {
        ASH_ASSERT(vaild(entity));
        return m_registry[entity.index];
    }

private:
    std::vector<entity_info> m_registry;
};
} // namespace ash::ecs