#include "ecs_command/ecs_command_system.hpp"

namespace violet
{
ecs_command_system::ecs_command_system()
    : engine_system("ECS Command")
{
}

bool ecs_command_system::initialize(const dictionary& config)
{
    return true;
}

void ecs_command_system::shutdown() {}

world_command* ecs_command_system::allocate_command()
{
    if (m_free_commands.empty())
    {
        m_commands.push_back(std::make_unique<world_command>());
        m_free_commands.push_back(m_commands.back().get());
    }

    world_command* command = m_free_commands.back();
    m_free_commands.pop_back();
    return command;
}

void ecs_command_system::execute(world_command* command, bool sync)
{
    if (sync)
    {
        get_world().execute(std::span(&command, 1));
        m_free_commands.push_back(command);
    }
    else
    {
        m_pending_commands.push_back(command);
    }
}
} // namespace violet