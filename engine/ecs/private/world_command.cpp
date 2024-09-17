#include "ecs/world_command.hpp"
#include "ecs/world.hpp"

namespace violet
{
world_command::world_command(const world* world)
    : m_world(world)
{
    m_chunks.push_back(std::make_unique<data_chunk>());
}

void world_command::reset()
{
    for (auto& command : m_commands)
    {
        if (command.component_data != nullptr)
        {
            auto builder = m_world->get_component_builder(command.component);
            builder->destruct(command.component_data);
        }
    }

    m_commands.clear();
    m_temp_entity_count = 0;
    m_chunk_offset = 0;
}

/*void* world_command::copy_component(component_id component, const void* component_data)
{
    auto builder = m_world->get_component_builder(component);

    void* pointer = allocate(builder->get_size(), builder->get_align());
    builder->copy_construct(component_data, pointer);

    return pointer;
}*/

void* world_command::move_component(component_id component, void* component_data)
{
    auto builder = m_world->get_component_builder(component);

    void* pointer = allocate(builder->get_size(), builder->get_align());
    builder->move_construct(component_data, pointer);

    return pointer;
}

void* world_command::allocate(std::size_t size, std::size_t align)
{
    auto align_offset = [](std::size_t offset, std::size_t align) -> std::size_t
    {
        return (offset + align - 1) & ~(align - 1);
    };

    assert(size < data_chunk::size);

    std::size_t offset = align_offset(m_chunk_offset, align);
    if (offset + size > data_chunk::size)
    {
        m_chunks.emplace_back(std::make_unique<data_chunk>());
        m_chunk_offset = size;
        return m_chunks.back()->get_data(0);
    }
    else
    {
        m_chunk_offset = offset + size;
        return m_chunks.back()->get_data(offset);
    }
}
} // namespace violet