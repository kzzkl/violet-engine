#include "context.hpp"
#include <filesystem>
#include <fstream>

namespace ash::core
{
system_base::system_base(std::string_view name) noexcept : m_name(name)
{
}

context::context(std::string_view config_path)
{
    load_config(config_path);

    std::size_t num_thread = m_config["core"]["threads"];
    if (num_thread == 0)
        num_thread = std::thread::hardware_concurrency();

    m_task = std::make_unique<ash::task::task_manager>(num_thread);
    m_task->schedule("root", []() {});

    m_world = std::make_unique<ash::ecs::world>();
    m_event = std::make_unique<event>();
    m_timer = std::make_unique<timer>();
}

void context::shutdown_system()
{
    m_task->stop();
    for (auto& system : m_systems)
    {
        system->shutdown();
    }
}

void context::load_config(std::string_view config_path)
{
    for (auto iter : std::filesystem::directory_iterator("engine/config"))
    {
        if (iter.is_regular_file() && iter.path().extension() == ".json")
        {
            std::ifstream fin(iter.path());
            if (!fin.is_open())
                continue;

            dictionary config;
            fin >> config;

            for (auto& [key, value] : config.items())
                m_config[key].update(value, true);
        }
    }

    for (auto iter : std::filesystem::directory_iterator(config_path))
    {
        if (iter.is_regular_file() && iter.path().extension() == ".json")
        {
            std::ifstream fin(iter.path());
            if (!fin.is_open())
                continue;

            dictionary config;
            fin >> config;

            for (auto& [key, value] : config.items())
                m_config[key].update(value, true);
        }
    }
}
} // namespace ash::core