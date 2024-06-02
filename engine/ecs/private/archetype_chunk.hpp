#pragma once

#include <array>
#include <memory>
#include <vector>

namespace violet
{
struct alignas(64) archetype_chunk
{
    static constexpr std::size_t CHUNK_SIZE = 1024 * 16;
    std::array<std::uint8_t, CHUNK_SIZE> data;
};

class archetype_chunk_allocator
{
public:
    archetype_chunk* allocate();
    void free(archetype_chunk* chunk);

private:
    std::vector<archetype_chunk*> m_free;
    std::vector<std::unique_ptr<archetype_chunk>> m_chunks;
};
} // namespace violet