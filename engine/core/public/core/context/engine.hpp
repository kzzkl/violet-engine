#pragma once

#include "common/assert.hpp"
#include "common/index_generator.hpp"
#include "common/log.hpp"
#include "core/context/engine_module.hpp"
#include "core/event/event.hpp"
#include "core/node/world.hpp"
#include "core/task/task_manager.hpp"
#include "core/timer/timer.hpp"
#include <memory>
#include <type_traits>
#include <vector>

namespace violet
{
template <typename T>
concept derived_from_module = std::is_base_of<engine_module, T>::value;

class engine
{
private:
    struct module_index : public index_generator<module_index, std::size_t>
    {
    };

public:
    static void initialize(std::string_view config_path);

    template <derived_from_module T, typename... Args>
    static void install(Args&&... args)
    {
        std::size_t index = module_index::value<T>();

        auto& singleton = instance();
        if (singleton.m_modules.size() <= index)
            singleton.m_modules.resize(index + 1);

        if (singleton.m_modules[index] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->initialize(singleton.m_config[m->get_name().data()]);
            log::info("Module installed successfully: {}.", m->get_name());
            singleton.m_modules[index] = std::move(m);

            singleton.m_install_sequence.push_back(index);
        }
        else
        {
            log::warn("The module is already installed.");
        }
    }

    template <derived_from_module T>
    static void uninstall()
    {
        std::size_t index = module_index::value<T>();
        auto& singleton = instance();
        singleton.uninstall(index);
    }

    static void run();
    static void exit();

    template <typename T>
    static T& get_module()
        requires derived_from_module<T>
    {
        auto pointer = instance().m_modules[module_index::value<T>()].get();
        VIOLET_ASSERT(dynamic_cast<T*>(pointer));
        return *static_cast<T*>(pointer);
    }

    static bool has_module(std::string_view name);

    static event& get_event() { return *instance().m_event; }
    static timer& get_timer() { return *instance().m_timer; }
    static world& get_world() { return *instance().m_world; }
    static task_manager& get_task_manager() { return *instance().m_task_manager; }

private:
    engine();
    static engine& instance();

    void uninstall(std::size_t index);

    void main_loop();

    void begin_frame();
    void end_frame();

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<engine_module>> m_modules;
    std::vector<std::size_t> m_install_sequence;

    std::unique_ptr<event> m_event;
    std::unique_ptr<timer> m_timer;
    std::unique_ptr<world> m_world;
    std::unique_ptr<task_manager> m_task_manager;

    std::atomic<bool> m_exit;
};
} // namespace violet