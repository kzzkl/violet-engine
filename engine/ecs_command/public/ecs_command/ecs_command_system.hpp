#pragma once

#include "core/engine_system.hpp"
#include <mutex>

namespace violet
{
class ecs_command_system : public engine_system
{
public:
    ecs_command_system();

    bool initialize(const dictionary& config) override;
    void shutdown() override;

    world_command* allocate_command();

private:
    void execute_commands();

    std::vector<std::unique_ptr<world_command>> m_commands;

    std::vector<world_command*> m_free_commands;
    std::vector<world_command*> m_pending_commands;

    std::mutex m_mutex;
};
} // namespace violet