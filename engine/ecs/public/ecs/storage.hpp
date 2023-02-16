#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace violet::ecs
{
static constexpr std::size_t CHUNK_SIZE = 1024 * 16;

class alignas(64) chunk
{
public:
    inline std::uint8_t* data() noexcept { return m_data.data(); }

private:
    std::array<std::uint8_t, CHUNK_SIZE> m_data;
};

class storage
{
public:
    void push_chunk() { m_chunks.push_back(std::make_unique<chunk>()); }
    void pop_chunk() noexcept { m_chunks.pop_back(); }

    void* get(std::size_t chunk_index, std::size_t offset = 0) noexcept
    {
        return m_chunks[chunk_index]->data() + offset;
    }

    void clear() noexcept { m_chunks.clear(); }

    std::size_t size() const noexcept { return m_chunks.size(); }

private:
    std::vector<std::unique_ptr<chunk>> m_chunks;
};
} // namespace violet::ecs