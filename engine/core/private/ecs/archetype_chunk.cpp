#include "ecs/archetype_chunk.hpp"
#include <cassert>

namespace violet
{
archetype_chunk* archetype_chunk_allocator::allocate()
{
    archetype_chunk* result;
    if (!m_free.empty())
    {
        result = m_free.back();
        m_free.pop_back();
    }
    else
    {
        m_chunks.emplace_back(std::make_unique<archetype_chunk>());
        result = m_chunks.back().get();
    }
    return result;
}

void archetype_chunk_allocator::free(archetype_chunk* chunk)
{
    assert(chunk != nullptr);
    m_free.push_back(chunk);
}
} // namespace violet