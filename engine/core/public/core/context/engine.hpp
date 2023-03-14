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

namespace violet::core
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
            m->initialize(singleton.m_config[m->name().data()]);
            log::info("Module installed successfully: {}.", m->name());
            singleton.m_modules[index] = std::move(m);

            singleton.m_installation_sequence.push_back(index);
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
        VIOLET_ASSERT(singleton.m_modules.size() > index);

        if (singleton.m_modules[index] != nullptr)
        {
            singleton.m_modules[index]->shutdown();
            singleton.m_modules[index] = nullptr;
            log::info("Module uninstalled successfully: {}.", singleton.m_modules[index]->name());
        }
        else
        {
            log::warn("The module is not installed.");
        }
    }

    static void run();
    static void exit();

    static void begin_frame();
    static void end_frame();

    template <typename T>
    static T& get_module()
        requires derived_from_module<T>
    {
        auto pointer = instance().m_modules[module_index::value<T>()].get();
        VIOLET_ASSERT(dynamic_cast<T*>(pointer));
        return *static_cast<T*>(pointer);
    }

    static event& get_event() { return *instance().m_event; }
    static timer& get_timer() { return *instance().m_timer; }
    static world& get_world() { return *instance().m_world; }
    static task_manager& get_task_manager() { return *instance().m_task_manager; }

private:
    engine();
    static engine& instance();

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<engine_module>> m_modules;
    std::vector<std::size_t> m_installation_sequence;

    std::unique_ptr<event> m_event;
    std::unique_ptr<timer> m_timer;
    std::unique_ptr<world> m_world;
    std::unique_ptr<task_manager> m_task_manager;

    std::atomic<bool> m_exit;
};
} // namespace violet::core