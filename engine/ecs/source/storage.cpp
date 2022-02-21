#include "storage.hpp"
#include <deque>

namespace ash::ecs
{

storage_handle::storage_handle() : storage_handle(nullptr, 0)
{
}

storage_handle::storage_handle(storage* owner, std::size_t index) : m_owner(owner), m_index(index)
{
}

void* storage_handle::get_component(std::size_t componentOffset, std::size_t componentSize)
{
    std::size_t entityPerChunk = m_owner->get_entity_per_chunk();
    chunk* chunk = m_owner->get_chunk(m_index / entityPerChunk);
    std::size_t offset = componentOffset + (m_index % entityPerChunk) * componentSize;
    return chunk->data() + offset;
}

storage_handle storage_handle::operator+(std::size_t offset)
{
    return storage_handle(m_owner, m_index + offset);
}

storage_handle storage_handle::operator-(std::size_t offset)
{
    return storage_handle(m_owner, m_index - offset);
}

std::size_t storage_handle::operator-(const storage_handle& other)
{
    ASH_ASSERT(m_owner == other.m_owner);
    return m_index - other.m_index;
}

bool storage_handle::operator==(const storage_handle& other)
{
    return m_owner == other.m_owner && m_index == other.m_index;
}

bool storage_handle::operator!=(const storage_handle& other)
{
    return !operator==(other);
}

storage::storage(std::size_t entity_per_chunk) : m_size(0), m_entity_per_chunk(entity_per_chunk)
{
}

storage::handle storage::insert_end()
{
    std::size_t index = get_entity_size();
    if (index >= get_capacity())
        push_chunk();

    ++m_size;
    return storage::handle(this, index);
}

void storage::erase_end()
{
    --m_size;
    if (m_size % m_entity_per_chunk == 0)
        pop_chunk();
}

chunk* storage::push_chunk()
{
    m_chunks.emplace_back(std::make_unique<chunk>());
    return m_chunks.back().get();
}

void storage::pop_chunk()
{
    m_chunks.pop_back();
}
} // namespace ash::ecs