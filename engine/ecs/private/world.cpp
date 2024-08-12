#include "ecs/world.hpp"
#include "archetype_chunk.hpp"

namespace violet
{
world::world()
{
    m_archetype_chunk_allocator = std::make_unique<archetype_chunk_allocator>();

    register_component<entity>();
}

world::~world()
{
    for (auto& [_, archetype] : m_archetypes)
    {
        archetype->clear();
    }
}

entity world::create()
{
    entity result;
    if (m_free_entity.empty())
    {
        result.id = m_entity_infos.size();
        m_entity_infos.emplace_back();
    }
    else
    {
        result.id = m_free_entity.front();
        m_free_entity.pop();
        result.version = m_entity_infos[result.id].version;
    }

    add_component<entity>(result);
    get_component<entity>(result) = result;

    return result;
}

void world::destroy(entity entity)
{
    entity_info& info = m_entity_infos[entity.id];
    if (info.archetype != nullptr)
    {
        info.archetype->remove(info.archetype_index);
        on_entity_move(entity.id, nullptr, 0);
    }

    info.archetype = nullptr;
    info.archetype_index = 0;
    ++info.version;

    m_free_entity.push(entity.id);
}

bool world::is_valid(entity entity) const
{
    if (entity.id < m_entity_infos.size())
        return m_entity_infos[entity.id].version == entity.version;
    else
        return false;
}

void world::on_entity_move(
    std::size_t entity_index, archetype* new_archetype, std::size_t new_archetype_index)
{
    entity_info& info = m_entity_infos[entity_index];
    if (info.archetype != nullptr && info.archetype->get_entity_count() > info.archetype_index)
    {
        auto [chunk_index, entity_offset] = std::div(
            static_cast<const long>(info.archetype_index),
            static_cast<const long>(info.archetype->entity_per_chunk()));

        std::size_t swap_entity_index =
            std::get<0>(
                info.archetype->get_components<entity>(chunk_index, entity_offset, m_world_version))
                ->id;
        m_entity_infos[swap_entity_index].archetype_index = info.archetype_index;
    }

    m_entity_infos[entity_index].archetype = new_archetype;
    m_entity_infos[entity_index].archetype_index = new_archetype_index;
}

archetype* world::make_archetype(std::span<const component_id> components)
{
    auto result = std::make_unique<archetype>(
        components, m_component_table, m_archetype_chunk_allocator.get());
    return (m_archetypes[result->get_mask()] = std::move(result)).get();
}
} // namespace violet