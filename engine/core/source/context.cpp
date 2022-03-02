#include "context.hpp"

namespace ash::core
{
context::context(const ash::common::dictionary& config) : m_config(config)
{
    std::size_t num_thread = std::thread::hardware_concurrency();

    auto config_iter = m_config.find("core");
    if (config_iter != m_config.end())
    {
        auto num_thread_iter = config_iter->find("number_of_threads");
        if (num_thread_iter != config_iter->end())
            num_thread = *num_thread_iter;
    }

    m_task = std::make_unique<ash::task::task_manager>(num_thread);
}

void context::initialize_submodule()
{
    for (auto& module : m_modules)
        module->initialize(m_config);
}
} // namespace ash::core