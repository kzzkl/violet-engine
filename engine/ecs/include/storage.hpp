#pragma once

#include "assert.hpp"
#include "component.hpp"
#include "ecs_exports.hpp"
#include <array>
#include <memory>
#include <vector>

namespace ash::ecs
{
class alignas(64) chunk
{
public:
    static constexpr std::size_t SIZE = 1024 * 16;

    uint8_t* data() { return m_data.data(); }

private:
    std::array<uint8_t, SIZE> m_data;
};

class storage;
class ECS_API storage_handle
{
public:
    storage_handle();
    storage_handle(storage* owner, std::size_t index);

    void* get_component(std::size_t componentOffset, std::size_t componentSize);

    storage_handle operator+(std::size_t offset);
    storage_handle operator-(std::size_t offset);
    std::size_t operator-(const storage_handle& other);
    bool operator==(const storage_handle& other);
    bool operator!=(const storage_handle& other);

private:
    storage* m_owner;
    std::size_t m_index;
};

class ECS_API storage
{
public:
    using handle = storage_handle;
    static constexpr std::size_t CHUNK_SIZE = chunk::SIZE;

public:
    storage(std::size_t entity_per_chunk);

    storage(storage&&) = default;
    storage& operator=(storage&&) = default;

    handle insert_end();
    void erase_end();

    chunk* get_chunk(std::size_t index) { return m_chunks[index].get(); }

    std::size_t get_entity_size() const { return m_size; }
    std::size_t get_chunk_size() const { return m_chunks.size(); }
    std::size_t get_entity_per_chunk() const { return m_entity_per_chunk; }

    bool empty() const { return m_size != 0; }

    handle begin() { return handle(this, 0); }
    handle end() { return handle(this, m_size); }

private:
    storage(const storage&) = delete;
    storage& operator=(const storage&) = delete;

    chunk* push_chunk();
    void pop_chunk();

    std::size_t get_capacity() const { return m_entity_per_chunk * m_chunks.size(); }

    std::vector<std::unique_ptr<chunk>> m_chunks;
    std::size_t m_size;
    std::size_t m_entity_per_chunk;
};
} // namespace ash::ecs