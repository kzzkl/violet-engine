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
private:
public:
    engine();
    engine(const engine&) = delete;
    ~engine();

    void initialize(std::string_view config_path);

    template <derived_from_system T, typename... Args>
    void install(Args&&... args)
    {
        install(engine_system_index::value<T>(), std::make_unique<T>(std::forward<Args>(args)...));
    }

    template <derived_from_system T>
    void uninstall()
    {
        uninstall(engine_system_index::value<T>());
    }

    void run();
    void exit();

    template <typename T>
    T& get_system()
    {
        return *static_cast<T*>(get_system(engine_system_index::value<T>()));
    }

    engine& operator=(const engine&) = delete;

private:
    void install(std::size_t index, std::unique_ptr<engine_system>&& system);
    void uninstall(std::size_t index);
    engine_system* get_system(std::size_t index);

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<engine_system>> m_systems;

    std::atomic<bool> m_exit;

    std::unique_ptr<engine_context> m_context;
};
} // namespace violet