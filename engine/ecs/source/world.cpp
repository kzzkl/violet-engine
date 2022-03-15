#include "world.hpp"

namespace ash::ecs
{
mask_archetype::mask_archetype(
    const archetype_layout& layout,
    const std::unordered_map<component_id, component_index>& index_map)
    : archetype(layout)
{
    for (const auto& [id, info] : layout)
        m_mask.set(index_map.at(id), true);
}

void mask_archetype::add(entity_record* record)
{
    archetype::add();

    record->archetype = this;
    record->index = m_record.size();

    m_record.push_back(record);
}

void mask_archetype::remove(std::size_t index)
{
    archetype::remove(index);

    std::swap(m_record[index], m_record.back());
    m_record[index]->index = index;
    m_record.pop_back();
}

void mask_archetype::move(std::size_t index, mask_archetype& target)
{
    archetype::move(index, target);

    entity_record* target_record = m_record[index];
    target_record->archetype = &target;
    target_record->index = target.m_record.size();
    target.m_record.push_back(target_record);

    std::swap(m_record[index], m_record.back());
    m_record[index]->index = index;
    m_record.pop_back();
}

world::world() noexcept
{
}

mask_archetype* world::create_archetype(const archetype_layout& layout)
{
    auto result = std::make_unique<mask_archetype>(layout, m_component_index);
    return (m_archetypes[result->get_mask()] = std::move(result)).get();
}
} // namespace ash::ecs