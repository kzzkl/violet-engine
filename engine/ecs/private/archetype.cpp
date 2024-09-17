#include "ecs/archetype.hpp"
#include "archetype_chunk.hpp"

namespace violet
{
archetype::archetype(
    std::span<const component_id> components,
    const component_builder_list& component_builder_list,
    archetype_chunk_allocator* allocator) noexcept
    : m_chunk_allocator(allocator)
{
    m_components.insert(m_components.end(), components.begin(), components.end());

    for (component_id id : components)
        m_mask.set(id);

    struct layout_info
    {
        component_id id;
        std::size_t size;
        std::size_t align;
    };

    std::vector<layout_info> list(components.size());
    std::transform(
        components.begin(),
        components.end(),
        list.begin(),
        [this, &component_builder_list](component_id id)
        {
            return layout_info{
                .id = id,
                .size = component_builder_list[id]->get_size(),
                .align = component_builder_list[id]->get_align()};
        });

    std::sort(
        list.begin(),
        list.end(),
        [](const auto& a, const auto& b)
        {
            return a.align == b.align ? a.id < b.id : a.align > b.align;
        });

    std::size_t entity_size = 0;
    for (const auto& info : list)
    {
        entity_size += info.size;
    }

    m_chunk_entity_count = archetype_chunk::size / entity_size;

    std::size_t offset = 0;
    for (std::size_t i = 0; i < list.size(); ++i)
    {
        component_id id = list[i].id;
        m_component_infos[id] = {
            .chunk_offset = offset,
            .index = i,
            .builder = component_builder_list[id].get()};

        offset += list[i].size * m_chunk_entity_count;
    }
}

archetype::~archetype()
{
    clear();
}

std::size_t archetype::add(std::uint32_t world_version)
{
    std::size_t index = allocate();

    auto [chunk_index, entity_index] =
        std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_entity_count));

    for (component_id id : m_components)
    {
        auto& info = m_component_infos[id];

        std::size_t offset = info.get_offset(entity_index);
        info.builder->construct(get_data_pointer(chunk_index, offset));

        set_version(chunk_index, world_version, id);
    }

    return index;
}

std::size_t archetype::move(std::size_t index, archetype& target, std::uint32_t world_version)
{
    assert(this != &target);

    auto& source = *this;

    auto [source_chunk_index, source_entity_index] =
        std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_entity_count));

    std::size_t target_index = target.allocate();
    auto [target_chunk_index, target_entity_index] = std::div(
        static_cast<const long>(target_index),
        static_cast<const long>(target.m_chunk_entity_count));

    for (component_id id : m_components)
    {
        if (target.m_mask.test(id))
        {
            auto& source_info = source.m_component_infos[id];
            auto& target_info = target.m_component_infos[id];

            std::size_t source_offset = source_info.get_offset(source_entity_index);
            std::size_t target_offset = target_info.get_offset(target_entity_index);
            source_info.builder->move_construct(
                source.get_data_pointer(source_chunk_index, source_offset),
                target.get_data_pointer(target_chunk_index, target_offset));

            std::uint32_t source_version = source.get_version(source_chunk_index, id);
            std::uint32_t target_version = target.get_version(target_chunk_index, id);
            if (source_version > target_version)
                target.set_version(target_chunk_index, source_version, id);
        }
    }

    for (component_id id : target.m_components)
    {
        if (!m_mask.test(id))
        {
            auto& target_info = target.m_component_infos[id];

            std::size_t offset = target_info.get_offset(target_entity_index);
            target_info.builder->construct(target.get_data_pointer(target_chunk_index, offset));

            target.set_version(target_chunk_index, world_version, id);
        }
    }

    remove(index);
    return target_index;
}

void archetype::remove(std::size_t index)
{
    assert(index < m_size);

    std::size_t back_index = m_size - 1;
    if (back_index == index)
    {
        destruct(index);
        --m_size;
    }
    else
    {
        move_assignment(back_index, index);
        destruct(back_index);
        --m_size;
    }

    if (m_size % m_chunk_entity_count == 0)
    {
        m_chunk_allocator->free(m_chunks.back());
        m_chunks.pop_back();
    }
}

void archetype::clear() noexcept
{
    for (std::size_t i = 0; i < m_size; ++i)
    {
        destruct(i);
    }

    for (archetype_chunk* chunk : m_chunks)
    {
        m_chunk_allocator->free(chunk);
    }

    m_chunks.clear();

    m_size = 0;
}

std::size_t archetype::allocate()
{
    std::size_t index = m_size;
    if (index >= capacity())
    {
        m_chunks.push_back(m_chunk_allocator->allocate());
        m_chunks.back()->component_versions.resize(m_components.size());
    }

    ++m_size;
    return index;
}

void archetype::move_construct(std::size_t source, std::size_t target)
{
    auto [source_chunk_index, source_entity_index] =
        std::div(static_cast<const long>(source), static_cast<const long>(m_chunk_entity_count));

    auto [target_chunk_index, target_entity_index] =
        std::div(static_cast<const long>(target), static_cast<const long>(m_chunk_entity_count));

    for (component_id id : m_components)
    {
        auto& info = m_component_infos[id];

        std::size_t source_offset = info.get_offset(source_entity_index);
        std::size_t target_offset = info.get_offset(target_entity_index);

        info.builder->move_construct(
            get_data_pointer(source_chunk_index, source_offset),
            get_data_pointer(target_chunk_index, target_offset));
    }
}

void archetype::destruct(std::size_t index)
{
    auto [chunk_index, entity_index] =
        std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_entity_count));

    for (component_id id : m_components)
    {
        auto& info = m_component_infos[id];

        std::size_t offset = info.get_offset(entity_index);
        info.builder->destruct(get_data_pointer(chunk_index, offset));
    }
}

void archetype::move_assignment(std::size_t source, std::size_t target)
{
    auto [source_chunk_index, source_entity_index] =
        std::div(static_cast<const long>(source), static_cast<const long>(m_chunk_entity_count));

    auto [target_chunk_index, target_entity_index] =
        std::div(static_cast<const long>(target), static_cast<const long>(m_chunk_entity_count));

    for (component_id id : m_components)
    {
        auto& info = m_component_infos[id];

        std::size_t source_offset = info.get_offset(source_entity_index);
        std::size_t target_offset = info.get_offset(target_entity_index);

        info.builder->move_assignment(
            get_data_pointer(source_chunk_index, source_offset),
            get_data_pointer(target_chunk_index, target_offset));
    }
}

void* archetype::get_data_pointer(std::size_t chunk_index, std::size_t offset)
{
    return &m_chunks[chunk_index]->data[offset];
}

void archetype::set_version(
    std::size_t chunk_index,
    std::uint32_t world_version,
    component_id component_id)
{
    std::size_t index = m_component_infos[component_id].index;
    m_chunks[chunk_index]->component_versions[index] = world_version;
}

std::uint32_t archetype::get_version(std::size_t chunk_index, component_id component_id)
{
    std::size_t index = m_component_infos[component_id].index;
    return m_chunks[chunk_index]->component_versions[index];
}

bool archetype::check_updated(
    std::size_t chunk_index,
    std::uint32_t system_version,
    component_id component_id)
{
    std::size_t index = m_component_infos[component_id].index;
    return system_version == 0 || m_chunks[chunk_index]->component_versions[index] > system_version;
}
} // namespace violet