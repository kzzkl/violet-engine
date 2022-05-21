#pragma once

#include "assert.hpp"
#include "dictionary.hpp"
#include "event.hpp"
#include "index_generator.hpp"
#include "log.hpp"
#include "task_manager.hpp"
#include "timer.hpp"
#include "world.hpp"
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ash::core
{
struct system_index : public index_generator<system_index, std::size_t>
{
};

class context;
class system_base
{
public:
    using context_type = context;

public:
    system_base(std::string_view name) noexcept;
    virtual ~system_base() = default;

    virtual bool initialize(const dictionary& config) = 0;
    virtual void shutdown() {}

    inline std::string_view name() const noexcept { return m_name; }

private:
    std::string m_name;
};

template <typename T>
concept internal_system =
    std::is_same_v<T, ash::task::task_manager> || std::is_same_v<T, ash::ecs::world> ||
    std::is_same_v<T, ash::core::timer> || std::is_same_v<T, event>;

template <typename T>
concept derived_from_system = std::is_base_of<system_base, T>::value;

class context
{
public:
    static void initialize(std::string_view config_path);

    template <derived_from_system T, typename... Args>
    static void install(Args&&... args)
    {
        std::size_t index = system_index::value<T>();

        auto& singleton = instance();
        if (singleton.m_systems.size() <= index)
            singleton.m_systems.resize(index + 1);

        if (singleton.m_systems[index] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->initialize(singleton.m_config[m->name().data()]);
            log::info("Module installed successfully: {}.", m->name());
            singleton.m_systems[index] = std::move(m);

            singleton.m_installation_sequence.push_back(index);
        }
        else
        {
            log::warn("The system is already installed.");
        }
    }

    template <derived_from_system T>
    static void uninstall()
    {
        std::size_t index = system_index::value<T>();

        auto& singleton = instance();
        ASH_ASSERT(singleton.m_systems.size() > index);

        if (singleton.m_systems[index] != nullptr)
        {
            singleton.m_systems[index]->shutdown();
            singleton.m_systems[index] = nullptr;
        }
        else
        {
            log::warn("The system is not installed.");
        }
    }

    static void shutdown();

    template <typename T>
    static T& system() requires derived_from_system<T> || internal_system<T>
    {
        return *static_cast<T*>(instance().m_systems[system_index::value<T>()].get());
    }

    template <>
    static ash::task::task_manager& system<ash::task::task_manager>()
    {
        return *instance().m_task;
    }

    template <>
    static ash::ecs::world& system<ash::ecs::world>()
    {
        return *instance().m_world;
    }

    template <>
    static ash::core::event& system<ash::core::event>()
    {
        return *instance().m_event;
    }

    template <>
    static ash::core::timer& system<ash::core::timer>()
    {
        return *instance().m_timer;
    }

private:
    context();
    static context& instance();

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<system_base>> m_systems;
    std::unique_ptr<ash::task::task_manager> m_task;
    std::unique_ptr<ash::ecs::world> m_world;
    std::unique_ptr<event> m_event;
    std::unique_ptr<timer> m_timer;

    std::vector<std::size_t> m_installation_sequence;
};
} // namespace ash::core

namespace ash
{
template <typename T>
T& system()
{
    return core::context::system<T>();
}
} // namespace ash