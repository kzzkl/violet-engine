#pragma once

#include "common/dictionary.hpp"
#include "common/type_index.hpp"
#include "core/timer.hpp"
#include "ecs/world.hpp"
#include "task/task_executor.hpp"

namespace violet
{
struct system_index : public type_index<system_index, std::size_t>
{
};

class application;
class engine_context;
class system
{
public:
    system(std::string_view name) noexcept;
    virtual ~system() = default;

    virtual void install(application& app) {}
    virtual bool initialize(const dictionary& config)
    {
        return true;
    }
    virtual void shutdown() {}

    inline const std::string& get_name() const noexcept
    {
        return m_name;
    }

protected:
    template <typename T>
    T& get_system()
    {
        return *static_cast<T*>(get_system(system_index::value<T>()));
    }

    timer& get_timer();
    world& get_world();

    task_graph& get_task_graph() noexcept;

    task_executor& get_executor() noexcept;

private:
    friend class application;

    system* get_system(std::size_t index);

    std::string m_name;
    engine_context* m_context;
};

template <typename T>
concept derived_from_system = std::is_base_of<system, T>::value;

class application
{
public:
    application(std::string_view config_path = "");
    ~application();

    template <derived_from_system T, typename... Args>
    void install(Args&&... args)
    {
        std::size_t index = system_index::value<T>();
        if (!has_system(index))
        {
            install(index, std::make_unique<T>(std::forward<Args>(args)...));
        }
    }

    void run();
    void exit();

private:
    void install(std::size_t index, std::unique_ptr<system>&& system);
    void uninstall(std::size_t index);

    bool has_system(std::size_t index) const noexcept;

    std::map<std::string, dictionary> m_config;
    std::vector<std::unique_ptr<system>> m_systems;

    std::unique_ptr<engine_context> m_context;
    std::atomic<bool> m_exit{true};
};
} // namespace violet