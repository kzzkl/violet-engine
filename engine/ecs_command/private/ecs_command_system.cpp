#include "ecs_command/ecs_command_system.hpp"

namespace violet
{
ecs_command_system::ecs_command_system()
    : engine_system("ECS Command")
{
}

bool ecs_command_system::initialize(const dictionary& config)
{
    task_graph& task_graph = get_task_graph();
    task_group& pre_update = task_graph.get_group("PreUpdate Group");
    task_group& update = task_graph.get_group("Update Group");
    task_group& post_update = task_graph.get_group("Post Update Group");

    task& pre_update_sync = task_graph.add_task()
                                .set_name("ECS Sync Point - PreUpdate")
                                .add_dependency(pre_update)
                                .set_options(TASK_OPTION_MAIN_THREAD)
                                .set_execute(
                                    [this]()
                                    {
                                        execute_commands();
                                    });
    update.add_dependency(pre_update_sync);

    task& update_sync = task_graph.add_task()
                            .set_name("ECS Sync Point - Update")
                            .add_dependency(update)
                            .set_options(TASK_OPTION_MAIN_THREAD)
                            .set_execute(
                                [this]()
                                {
                                    execute_commands();
                                });
    post_update.add_dependency(update_sync);

    task& post_update_sync = task_graph.add_task()
                                 .set_name("ECS Sync Point - Post Update")
                                 .add_dependency(post_update)
                                 .set_options(TASK_OPTION_MAIN_THREAD)
                                 .set_execute(
                                     [this]()
                                     {
                                         execute_commands();
                                     });

    return true;
}

void ecs_command_system::shutdown() {}

world_command* ecs_command_system::allocate_command()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_free_commands.empty())
    {
        m_commands.push_back(std::make_unique<world_command>(&get_world()));
        m_free_commands.push_back(m_commands.back().get());
    }

    world_command* command = m_free_commands.back();
    m_free_commands.pop_back();

    m_pending_commands.push_back(command);

    return command;
}

void ecs_command_system::execute_commands()
{
    get_world().execute(m_pending_commands);

    for (world_command* command : m_pending_commands)
    {
        command->reset();
        m_free_commands.push_back(command);
    }

    m_pending_commands.clear();
}
} // namespace violet