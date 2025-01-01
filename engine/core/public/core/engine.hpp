#pragma once

#include "core/engine_system.hpp"
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
public:
    static void initialize(std::string_view config_path);

    template <derived_from_system T, typename... Args>
    static void install(Args&&... args)
    {
        instance().install(
            engine_system_index::value<T>(),
            std::make_unique<T>(std::forward<Args>(args)...));
    }

    template <derived_from_system T>
    static void uninstall()
    {
        instance().uninstall(engine_system_index::value<T>());
    }

    static void run();
    static void exit();

    engine& operator=(const engine&) = delete;

private:
    engine();
    engine(const engine&) = delete;
    ~engine();

    static engine& instance();

    void install(std::size_t index, std::unique_ptr<engine_system>&& system);
    void uninstall(std::size_t index);

    std::map<std::string, dictionary> m_config;
    std::vector<std::unique_ptr<engine_system>> m_systems;

    std::unique_ptr<engine_context> m_context;

    std::atomic<bool> m_exit;
};
} // namespace violet