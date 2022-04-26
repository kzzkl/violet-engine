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
    virtual void shutdown(){};

    inline std::string_view name() const noexcept { return m_name; }

protected:
    template <typename T>
    T& system() const;

    context_type* context() const noexcept { return m_context; }

private:
    friend class context_type;

    std::string m_name;
    context_type* m_context;
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
    context(std::string_view config_path);

    template <typename T>
    T& system() const requires derived_from_system<T> || internal_system<T>
    {
        return *static_cast<T*>(m_systems[system_index::value<T>()].get());
    }

    template <>
    ash::task::task_manager& system<ash::task::task_manager>() const
    {
        return *m_task;
    }

    template <>
    ash::ecs::world& system<ash::ecs::world>() const
    {
        return *m_world;
    }

    template <>
    ash::core::event& system<ash::core::event>() const
    {
        return *m_event;
    }

    template <>
    ash::core::timer& system<ash::core::timer>() const
    {
        return *m_timer;
    }

protected:
    template <derived_from_system T, typename... Args>
    void install_system(Args&&... args)
    {
        std::size_t index = system_index::value<T>();
        if (m_systems.size() <= index)
            m_systems.resize(index + 1);

        if (m_systems[index] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->m_context = this;
            m->initialize(m_config[m->name().data()]);
            log::info("Module installed successfully: {}.", m->name());
            m_systems[index] = std::move(m);
        }
        else
        {
            log::warn("The system is already installed.");
        }
    }

    void shutdown_system();

private:
    void load_config(std::string_view config_path);

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<system_base>> m_systems;
    std::unique_ptr<ash::task::task_manager> m_task;
    std::unique_ptr<ash::ecs::world> m_world;
    std::unique_ptr<event> m_event;
    std::unique_ptr<timer> m_timer;
};

template <typename T>
T& system_base::system() const
{
    return m_context->system<T>();
}
} // namespace ash::core