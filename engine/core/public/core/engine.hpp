#pragma once

#include "common/log.hpp"
#include "common/type_index.hpp"
#include "core/engine_system.hpp"
#include "core/node/world.hpp"
#include "core/task/task_executor.hpp"
#include "core/timer.hpp"
#include <cassert>
#include <memory>
#include <type_traits>
#include <vector>

namespace violet
{
template <typename T>
concept derived_from_system = std::is_base_of<engine_system, T>::value;

class engine
{
private:
    struct system_index : public type_index<system_index, std::size_t>
    {
    };

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
            m->initialize(singleton.m_config[m->get_name().data()]);
            log::info("System installed successfully: {}.", m->get_name());
            singleton.m_systems[index] = std::move(m);

            singleton.m_install_sequence.push_back(index);
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
        singleton.uninstall(index);
    }

    static void run();
    static void exit();

    template <typename T>
    static T& get_system()
        requires derived_from_system<T>
    {
        auto pointer = instance().m_systems[system_index::value<T>()].get();
        assert(dynamic_cast<T*>(pointer));
        return *static_cast<T*>(pointer);
    }

    static bool has_system(std::string_view name);

    static timer& get_timer() { return *instance().m_timer; }
    static world& get_world() { return *instance().m_world; }

    static task_executor& get_task_executor() { return *instance().m_task_executor; }

    static task<>& on_frame_begin() { return instance().m_frame_begin.get_root(); }
    static task<>& on_frame_end() { return instance().m_frame_end.get_root(); }
    static task<float>& on_tick() { return instance().m_tick.get_root(); }

private:
    engine();
    static engine& instance();

    void uninstall(std::size_t index);

    void main_loop();

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<engine_system>> m_systems;
    std::vector<std::size_t> m_install_sequence;

    std::unique_ptr<timer> m_timer;
    std::unique_ptr<world> m_world;

    std::unique_ptr<task_executor> m_task_executor;

    task_graph<> m_frame_begin;
    task_graph<float> m_tick;
    task_graph<> m_frame_end;

    std::atomic<bool> m_exit;
};
} // namespace violet