#pragma once

#include <array>
#include <memory>
#include <vector>

namespace violet
{
struct alignas(64) archetype_chunk
{
    static constexpr std::size_t size = 1024ull * 16;
    std::array<std::uint8_t, size> data;

    std::vector<std::uint32_t> component_versions;

    void set_version(std::size_t component_index, std::uint32_t version)
    {
        component_versions[component_index] = version;
    }
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