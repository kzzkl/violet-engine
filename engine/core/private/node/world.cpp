#include "core/node/world.hpp"

namespace violet::core
{
world::world()
{
}

world::~world()
{
    for (auto& [_, archetype] : m_archetypes)
        archetype->clear();
}

entity world::create()
{
    entity result;
    if (m_free_entity.empty())
    {
        result.index = m_entity_infos.size();
        m_entity_infos.emplace_back();
    }
    else
    {
        result.index = m_free_entity.front();
        m_free_entity.pop();
        result.entity_version = m_entity_infos[result.index].entity_version;
    }

    return result;
}

void world::release(entity entity)
{
    entity_info& info = m_entity_infos[entity.index];
    if (info.archetype != nullptr)
        info.archetype->remove(info.archetype_index);

    info.archetype = nullptr;
    info.archetype_index = 0;
    ++info.entity_version;

    m_free_entity.push(entity.index);
}

std::pair<bool, bool> world::is_valid(entity entity) const
{
    if (entity.index < m_entity_infos.size())
        return {
            m_entity_infos[entity.index].entity_version == entity.entity_version,
            m_entity_infos[entity.index].component_version == entity.component_version};
    else
        return {false, false};
}

std::pair<std::uint16_t, std::uint16_t> world::get_version(entity entity) const
{
    const entity_info& info = m_entity_infos[entity.index];
    return {info.entity_version, info.component_version};
}

void world::update_entity_index(
    std::size_t entity_index,
    archetype* new_archetype,
    std::size_t new_archetype_index)
{
    entity_info& info = m_entity_infos[entity_index];

    if (info.archetype != nullptr)
    {
        auto& old_index_map = m_archetype_index_map[info.archetype];
        std::size_t move_entity_index = old_index_map.back();
        if (move_entity_index != info.archetype_index)
        {
            m_entity_infos[move_entity_index].archetype_index = info.archetype_index;
            old_index_map[info.archetype_index] = move_entity_index;
        }

        old_index_map.pop_back();
    }

    auto& new_index_map = m_archetype_index_map[new_archetype];
    if (new_index_map.size() <= new_archetype_index)
        new_index_map.resize(new_archetype_index + 1);
    new_index_map[new_archetype_index] = entity_index;

    info.archetype = new_archetype;
    info.archetype_index = new_archetype_index;
    ++info.component_version;
}

archetype* world::make_archetype(const std::vector<component_id>& components)
{
    auto result = std::make_unique<archetype>(components, m_component_infos);
    return (m_archetypes[result->mask()] = std::move(result)).get();
}
} // namespace violet::core