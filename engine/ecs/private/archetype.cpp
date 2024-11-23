#include "ecs/archetype.hpp"
#include "archetype_chunk.hpp"

namespace violet
{
archetype::archetype(const archetype_layout& layout, archetype_chunk_allocator* allocator) noexcept
    : m_chunk_allocator(allocator)
{
    assert(layout.size() < std::numeric_limits<std::uint8_t>::max());

    m_components.reserve(layout.size());
    for (auto& [id, builder] : layout)
    {
        m_components.push_back({
            .id = id,
            .builder = builder,
        });

        m_mask.set(id);
    }

    std::sort(
        m_components.begin(),
        m_components.end(),
        [](const component_info& a, const component_info& b)
        {
            std::size_t a_align = a.builder->get_align();
            std::size_t b_align = b.builder->get_align();
            return a_align == b_align ? a.id < b.id : a_align > b_align;
        });

    std::size_t entity_size = 0;
    for (const auto& component : m_components)
    {
        entity_size += component.builder->get_size();
    }

    m_chunk_capacity = archetype_chunk::size / entity_size;

    std::size_t chunk_offset = 0;
    for (auto& component : m_components)
    {
        component.chunk_offset = chunk_offset;
        chunk_offset += component.builder->get_size() * m_chunk_capacity;
    }

    for (std::size_t i = 0; i < m_components.size(); ++i)
    {
        m_component_id_to_index[m_components[i].id] = static_cast<std::uint8_t>(i);
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
        std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_capacity));

    for (auto& component : m_components)
    {
        component.builder->construct(
            get_data_pointer(chunk_index, component.get_offset(entity_index)));

        set_version(chunk_index, world_version, component.id);
    }

    return index;
}

std::size_t archetype::move(std::size_t index, archetype& dst, std::uint32_t world_version)
{
    assert(this != &dst);

    auto& src = *this;

    auto [src_chunk_index, src_entity_index] =
        std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_capacity));

    std::size_t dst_index = dst.allocate();
    auto [dst_chunk_index, dst_entity_index] =
        std::div(static_cast<const long>(dst_index), static_cast<const long>(dst.m_chunk_capacity));

    for (auto& src_info : m_components)
    {
        component_id id = src_info.id;

        if (dst.m_mask.test(id))
        {
            auto& dst_info = dst.get_component_info(id);

            std::size_t src_offset = src_info.get_offset(src_entity_index);
            std::size_t dst_offset = dst_info.get_offset(dst_entity_index);
            src_info.builder->move_construct(
                src.get_data_pointer(src_chunk_index, src_offset),
                dst.get_data_pointer(dst_chunk_index, dst_offset));

            std::uint32_t src_version = src.get_version(src_chunk_index, id);
            std::uint32_t dst_version = dst.get_version(dst_chunk_index, id);
            if (src_version > dst_version)
            {
                dst.set_version(dst_chunk_index, src_version, id);
            }
        }
    }

    for (auto& dst_info : dst.m_components)
    {
        component_id id = dst_info.id;

        if (!m_mask.test(id))
        {
            std::size_t offset = dst_info.get_offset(dst_entity_index);
            dst_info.builder->construct(dst.get_data_pointer(dst_chunk_index, offset));

            dst.set_version(dst_chunk_index, world_version, id);
        }
    }

    remove(index);
    return dst_index;
}

void archetype::remove(std::size_t index)
{
    assert(index < m_entity_count);

    std::size_t back_index = m_entity_count - 1;
    if (back_index == index)
    {
        destruct(index);
        --m_entity_count;
    }
    else
    {
        move_assignment(back_index, index);
        destruct(back_index);
        --m_entity_count;
    }

    if (m_entity_count % m_chunk_capacity == 0)
    {
        m_chunk_allocator->free(m_chunks.back());
        m_chunks.pop_back();
    }
}

void archetype::clear() noexcept
{
    for (std::size_t i = 0; i < m_entity_count; ++i)
    {
        destruct(i);
    }

    for (archetype_chunk* chunk : m_chunks)
    {
        m_chunk_allocator->free(chunk);
    }

    m_chunks.clear();

    m_entity_count = 0;
}

std::size_t archetype::allocate()
{
    std::size_t index = m_entity_count;
    if (index >= get_capacity())
    {
        m_chunks.push_back(m_chunk_allocator->allocate());
        m_chunks.back()->component_versions.resize(m_components.size());
    }

    ++m_entity_count;
    return index;
}

void archetype::move_construct(std::size_t src, std::size_t dst)
{
    auto [src_chunk_index, src_entity_index] =
        std::div(static_cast<const long>(src), static_cast<const long>(m_chunk_capacity));

    auto [dst_chunk_index, dst_entity_index] =
        std::div(static_cast<const long>(dst), static_cast<const long>(m_chunk_capacity));

    for (auto& component : m_components)
    {
        std::size_t src_offset = component.get_offset(src_entity_index);
        std::size_t dst_offset = component.get_offset(dst_entity_index);

        component.builder->move_construct(
            get_data_pointer(src_chunk_index, src_offset),
            get_data_pointer(dst_chunk_index, dst_offset));
    }
}

void archetype::destruct(std::size_t index)
{
    auto [chunk_index, entity_index] =
        std::div(static_cast<const long>(index), static_cast<const long>(m_chunk_capacity));

    for (auto& component : m_components)
    {
        std::size_t offset = component.get_offset(entity_index);
        component.builder->destruct(get_data_pointer(chunk_index, offset));
    }
}

void archetype::move_assignment(std::size_t src, std::size_t dst)
{
    auto [src_chunk_index, src_entity_index] =
        std::div(static_cast<const long>(src), static_cast<const long>(m_chunk_capacity));

    auto [dst_chunk_index, dst_entity_index] =
        std::div(static_cast<const long>(dst), static_cast<const long>(m_chunk_capacity));

    for (auto& component : m_components)
    {
        std::size_t src_offset = component.get_offset(src_entity_index);
        std::size_t dst_offset = component.get_offset(dst_entity_index);

        component.builder->move_assignment(
            get_data_pointer(src_chunk_index, src_offset),
            get_data_pointer(dst_chunk_index, dst_offset));
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
    std::size_t index = m_component_id_to_index[component_id];
    m_chunks[chunk_index]->component_versions[index] = world_version;
}

std::uint32_t archetype::get_version(std::size_t chunk_index, component_id component_id)
{
    std::size_t index = m_component_id_to_index[component_id];
    return m_chunks[chunk_index]->component_versions[index];
}

bool archetype::check_updated(
    std::size_t chunk_index,
    std::uint32_t system_version,
    component_id component_id)
{
    std::size_t index = m_component_id_to_index[component_id];
    return system_version == 0 || m_chunks[chunk_index]->component_versions[index] > system_version;
}
} // namespace violet