#include "core/node/world.hpp"
#include "node/archetype_chunk.hpp"

namespace violet
{
world::world() : m_view_version(0)
{
    m_archetype_chunk_allocator = std::make_unique<archetype_chunk_allocator>();

    register_component<node*>();
    register_component<entity_record>();
}

world::~world()
{
    for (auto& [_, archetype] : m_archetypes)
        archetype->clear();
}

entity world::create(node* owner)
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

    add_component<node*, entity_record>(result);
    get_component<node*>(result) = owner;
    get_component<entity_record>(result).entity_index = result.index;

    return result;
}

void world::release(entity entity)
{
    entity_info& info = m_entity_infos[entity.index];
    if (info.archetype != nullptr)
    {
        info.archetype->remove(info.archetype_index);
        on_entity_move(entity.index, nullptr, 0);
    }

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

void world::on_entity_move(
    std::size_t entity_index,
    archetype* new_archetype,
    std::size_t new_archetype_index)
{
    entity_info& info = m_entity_infos[entity_index];
    if (info.archetype != nullptr && info.archetype->size() > info.archetype_index)
    {
        auto iter = info.archetype->begin() + info.archetype_index;
        std::size_t swap_entity_index = iter.get_component<entity_record>().entity_index;
        m_entity_infos[swap_entity_index].archetype_index = info.archetype_index;
        ++m_entity_infos[swap_entity_index].component_version;
    }

    m_entity_infos[entity_index].archetype = new_archetype;
    m_entity_infos[entity_index].archetype_index = new_archetype_index;
    ++m_entity_infos[entity_index].component_version;
}

archetype* world::make_archetype(const std::vector<component_id>& components)
{
    auto result = std::make_unique<archetype>(
        components,
        m_component_infos,
        m_archetype_chunk_allocator.get());
    return (m_archetypes[result->get_mask()] = std::move(result)).get();
}
} // namespace violet