#pragma once

#include "core/engine_system.hpp"

namespace violet
{
class ecs_command_system : public engine_system
{
public:
    ecs_command_system();

    bool initialize(const dictionary& config) override;
    void shutdown() override;

    world_command* allocate_command();

    void execute(world_command* command, bool sync = false);

private:
    std::vector<std::unique_ptr<world_command>> m_commands;

    std::vector<world_command*> m_free_commands;
    std::vector<world_command*> m_pending_commands;
};
} // namespace violet