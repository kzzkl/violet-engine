#include "application.hpp"

using namespace ash::common;
using namespace ash::task;

namespace ash::core
{
application::application(const ash::common::dictionary& config) : m_config(config)
{
    std::size_t num_thread = std::thread::hardware_concurrency();

    auto config_iter = m_config.find("core");
    if (config_iter != m_config.end())
    {
        auto num_thread_iter = config_iter->find("number_of_threads");
        if (num_thread_iter != config_iter->end())
            num_thread = *num_thread_iter;
    }

    m_task = std::make_unique<task_manager>(num_thread);
}

void application::run()
{
    for (auto& module : m_modules)
    {
        module->initialize(m_config);
    }

    for (auto& module : m_modules)
    {
        auto handle = m_task->schedule(module->get_name(), [&module]() { module->tick(); });
    }

    m_task->run();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(10));
}
} // namespace ash::core