#include "context.hpp"
#include <filesystem>
#include <fstream>

namespace ash::core
{
submodule::submodule(std::string_view name) noexcept : m_name(name)
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
    m_timer = std::make_unique<timer>();
}

void context::shutdown_submodule()
{
    m_task->stop();
    for (auto& [id, submodule] : m_modules)
    {
        submodule->shutdown();
    }
}

void context::load_config(std::string_view config_path)
{
    std::filesystem::path path = config_path;
    std::filesystem::path default_path = path / "default";

    for (auto iter : std::filesystem::directory_iterator(default_path))
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

    for (auto iter : std::filesystem::directory_iterator(path))
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