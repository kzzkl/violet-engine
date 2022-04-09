#pragma once

#include "archetype_wrapper.hpp"

namespace ash::ecs
{
class entity_record
{
public:
    using archetype_type = archetype_wrapper;
    using entity_type = entity;

    struct record_item
    {
        std::uint32_t version;

        archetype_type* archetype;
        std::size_t index;
    };

public:
    entity_type add()
    {
        entity_type result = {static_cast<std::uint32_t>(m_record.size()), 0};
        m_record.emplace_back();
        return result;
    }

    void update(const archetype_change_list& list)
    {
        for (auto& item : list)
        {
            m_record[item.entity.id].archetype = item.archetype;
            m_record[item.entity.id].index = item.index;

            ++m_record[item.entity.id].version;
        }
    }

    bool vaild(entity_type entity) const noexcept
    {
        return m_record[entity.id].version == entity.version;
    }

    const record_item& operator[](entity_type entity) const noexcept { return m_record[entity.id]; }

private:
    std::vector<record_item> m_record;
};
} // namespace ash::ecs