#pragma once

#include "ecs/view.hpp"

namespace violet
{
class world;
class world_command
{
public:
    enum command_type : std::uint8_t
    {
        COMMAND_CREATE,
        COMMAND_DESTROY,
        COMMAND_ADD_COMPONENT,
        COMMAND_REMOVE_COMPONENT
    };

    struct command
    {
        entity e;
        command_type type;

        component_id component;
        void* component_data{nullptr};
    };

public:
    entity create()
    {
        command cmd = {};
        cmd.type = COMMAND_ADD_COMPONENT;
        cmd.e.id = m_temp_entity_count;
        cmd.e.type = ENTITY_TEMPORARY;
        m_commands.push_back(cmd);

        ++m_temp_entity_count;
    }

    void destroy(entity e)
    {
        command cmd = {};
        cmd.type = COMMAND_ADD_COMPONENT;
        cmd.e = e;
        m_commands.push_back(cmd);
    }

    template <typename Component>
    void add_component(entity e)
    {
        command cmd = {};
        cmd.type = COMMAND_ADD_COMPONENT;
        cmd.e = e;
        cmd.component = component_index::value<Component>();
        m_commands.push_back(cmd);
    }

    template <typename Component>
    void add_component(entity e, Component&& component)
    {
        command cmd = {};
        cmd.type = COMMAND_ADD_COMPONENT;
        cmd.e = e;
        cmd.component = component_index::value<Component>();
        cmd.component_data = record_component(cmd.component, &component);
        m_commands.push_back(cmd);
    }

    template <typename Component>
    void remove_component(entity e)
    {
        command cmd = {};
        cmd.type = COMMAND_ADD_COMPONENT;
        cmd.e = e;
        cmd.component = component_index::value<Component>();
        m_commands.push_back(cmd);
    }

    const std::vector<command>& get_commands() const noexcept
    {
        return m_commands;
    }

    std::size_t get_temp_entity_count() const noexcept
    {
        return m_temp_entity_count;
    }

    void reset();

private:
    struct alignas(64) data_chunk
    {
        static constexpr std::size_t size = 1024 * 16;
        std::array<std::uint8_t, size> data;

        void* get_data(std::size_t offset)
        {
            return &data[offset];
        }
    };

    void* record_component(component_id component, void* component_data);
    void* allocate(std::size_t size, std::size_t align);

    std::vector<command> m_commands;
    std::uint32_t m_temp_entity_count{0};

    std::vector<std::unique_ptr<data_chunk>> m_chunks;
    std::size_t m_chunk_offset;

    world* m_world;
};
} // namespace violet