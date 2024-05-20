#pragma once

#include "core/engine_module.hpp"
#include <cassert>
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
public:
    engine();
    engine(const engine&) = delete;
    ~engine();

    void initialize(std::string_view config_path);

    template <derived_from_module T, typename... Args>
    void install(Args&&... args)
    {
        install(engine_module_index::value<T>(), std::make_unique<T>(std::forward<Args>(args)...));
    }

    template <derived_from_module T>
    void uninstall()
    {
        uninstall(engine_module_index::value<T>());
    }

    void run();
    void exit();

    template <typename T>
    T& get_module()
    {
        return *static_cast<T*>(get_module(engine_module_index::value<T>()));
    }

    engine& operator=(const engine&) = delete;

private:
    void install(std::size_t index, std::unique_ptr<engine_module>&& module);
    void uninstall(std::size_t index);
    engine_module* get_module(std::size_t index);

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<engine_module>> m_modules;

    std::atomic<bool> m_exit;

    std::unique_ptr<engine_context> m_context;
};
} // namespace violet