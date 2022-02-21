#include "application.hpp"

using namespace ash::common;

namespace ash::core
{
application::application(const ash::common::dictionary& config) : m_config(config)
{
}

void application::run()
{
    for (auto& module : m_modules)
    {
        module->initialize(m_config);
    }

    while (true)
    {
        for (auto& module : m_modules)
        {
            module->tick();
        }

        std::this_thread::yield();
    }
}
} // namespace ash::core