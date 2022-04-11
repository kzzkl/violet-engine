#pragma once

#include "assert.hpp"
#include "dictionary.hpp"
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
namespace internal
{
struct system_id_generator
{
    static std::size_t next() noexcept
    {
        static std::size_t id = 0;
        return id++;
    }
};
} // namespace internal

template <typename T>
struct system_id
{
    static const std::size_t value() noexcept
    {
        static const std::size_t id = internal::system_id_generator::next();
        return id;
    }
};

class context;
class system_base
{
public:
    system_base(std::string_view name) noexcept;
    virtual ~system_base() = default;

    virtual bool initialize(const dictionary& config) = 0;
    virtual void shutdown(){};

    inline std::string_view name() const noexcept { return m_name; }

protected:
    template <typename T>
    T& system();

private:
    friend class context;

    std::string m_name;
    context* m_context;
};

template <typename T>
concept internal_system = std::is_same_v<T, ash::task::task_manager> ||
    std::is_same_v<T, ash::ecs::world> || std::is_same_v<T, ash::core::timer>;

template <typename T>
concept derived_from_system = std::is_base_of<system_base, T>::value;

class context
{
public:
    context(std::string_view config_path);

    template <typename T>
    T& system() requires derived_from_system<T> || internal_system<T>
    {
        return *static_cast<T*>(m_systems[system_id<T>::value()].get());
    }

    template <>
    ash::task::task_manager& system<ash::task::task_manager>()
    {
        return *m_task;
    }

    template <>
    ash::ecs::world& system<ash::ecs::world>()
    {
        return *m_world;
    }

    template <>
    ash::core::timer& system<ash::core::timer>()
    {
        return *m_timer;
    }

protected:
    template <derived_from_system T, typename... Args>
    void install_system(Args&&... args)
    {
        std::size_t id = system_id<T>::value();
        if (m_systems.size() <= id)
            m_systems.resize(id + 1);

        if (m_systems[id] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->m_context = this;
            m->initialize(m_config[m->name().data()]);
            log::info("Module installed successfully: {}.", m->name());
            m_systems[id] = std::move(m);
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

    std::unique_ptr<timer> m_timer;
};

template <typename T>
T& system_base::system()
{
    return m_context->system<T>();
}
} // namespace ash::core