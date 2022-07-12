#include "core/context.hpp"
#include <filesystem>
#include <fstream>

namespace ash::core
{
system_base::system_base(std::string_view name) noexcept : m_name(name)
{
}

context::context()
{
}

context& context::instance()
{
    static context instance;
    return instance;
}

void context::initialize(std::string_view config_path)
{
    auto& singleton = instance();

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
                singleton.m_config[key].update(value, true);
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
                singleton.m_config[key].update(value, true);
        }
    }
}

void context::shutdown()
{
    auto& singleton = instance();

    for (auto iter = singleton.m_installation_sequence.rbegin();
         iter != singleton.m_installation_sequence.rend();
         ++iter)
    {
        log::info("Module shutdown: {}.", singleton.m_systems[*iter]->name());
        singleton.m_systems[*iter]->shutdown();
        singleton.m_systems[*iter] = nullptr;
    }
}

void context::begin_frame()
{
    auto& singleton = instance();

    for (auto& system : singleton.m_systems)
        system->on_begin_frame();
}

void context::end_frame()
{
    auto& singleton = instance();

    for (auto& system : singleton.m_systems)
        system->on_end_frame();
}
} // namespace ash::core