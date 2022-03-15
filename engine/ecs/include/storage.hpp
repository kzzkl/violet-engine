#pragma once

#include "assert.hpp"
#include "component.hpp"
#include <array>
#include <memory>
#include <vector>

namespace ash::ecs
{
class alignas(64) chunk
{
public:
    static constexpr std::size_t SIZE = 1024 * 16;

    inline uint8_t* data() noexcept { return m_data.data(); }

private:
    std::array<uint8_t, SIZE> m_data;
};

class storage;
class storage_handle
{
public:
    storage_handle() noexcept : storage_handle(nullptr, 0) {}
    storage_handle(storage* owner, std::size_t index) noexcept : m_owner(owner), m_index(index) {}

    void* get_component(std::size_t componentOffset, std::size_t componentSize);

    storage_handle operator+(std::size_t offset) noexcept
    {
        return storage_handle(m_owner, m_index + offset);
    }

    storage_handle operator-(std::size_t offset) noexcept
    {
        return storage_handle(m_owner, m_index - offset);
    }

    std::size_t operator-(const storage_handle& other) noexcept
    {
        ASH_ASSERT(m_owner == other.m_owner);
        return m_index - other.m_index;
    }

    bool operator==(const storage_handle& other) noexcept
    {
        return m_owner == other.m_owner && m_index == other.m_index;
    }

    bool operator!=(const storage_handle& other) noexcept { return !operator==(other); }

private:
    storage* m_owner;
    std::size_t m_index;
};

class storage
{
public:
    using handle = storage_handle;
    static constexpr std::size_t CHUNK_SIZE = chunk::SIZE;

public:
    storage(std::size_t entity_per_chunk) noexcept : m_size(0), m_entity_per_chunk(entity_per_chunk)
    {
    }

    storage(const storage&) = delete;
    storage(storage&&) noexcept = default;
    storage& operator=(storage&&) noexcept = default;

    handle push_back()
    {
        std::size_t index = get_entity_size();
        if (index >= get_capacity())
            m_chunks.push_back(std::make_unique<chunk>());

        ++m_size;
        return storage::handle(this, index);
    }

    void pop_back()
    {
        --m_size;
        if (m_size % m_entity_per_chunk == 0)
            m_chunks.pop_back();
    }

    chunk* get_chunk(std::size_t index) { return m_chunks[index].get(); }

    std::size_t get_entity_size() const { return m_size; }
    std::size_t get_chunk_size() const { return m_chunks.size(); }
    std::size_t get_entity_per_chunk() const { return m_entity_per_chunk; }

    inline bool empty() const noexcept { return m_size != 0; }

    inline handle begin() noexcept { return handle(this, 0); }
    inline handle end() noexcept { return handle(this, m_size); }

    inline std::size_t size() const noexcept { return m_size; }

    storage& operator=(const storage&) = delete;

private:
    inline std::size_t get_capacity() const noexcept
    {
        return m_entity_per_chunk * m_chunks.size();
    }

    std::vector<std::unique_ptr<chunk>> m_chunks;
    std::size_t m_size;
    std::size_t m_entity_per_chunk;
};

inline void* storage_handle::get_component(std::size_t componentOffset, std::size_t componentSize)
{
    std::size_t entityPerChunk = m_owner->get_entity_per_chunk();
    chunk* chunk = m_owner->get_chunk(m_index / entityPerChunk);
    std::size_t offset = componentOffset + (m_index % entityPerChunk) * componentSize;
    return chunk->data() + offset;
}
} // namespace ash::ecs