#pragma once

#include "assert.hpp"
#include "dictionary.hpp"
#include "index_generator.hpp"
#include "log.hpp"
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

    virtual bool initialize(const dictionary& config) { return true; }
    virtual void shutdown() {}

    virtual void on_begin_frame() {}
    virtual void on_end_frame() {}

    inline std::string_view name() const noexcept { return m_name; }

private:
    std::string m_name;
};

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
            log::info("Module uninstalled successfully: {}.", singleton.m_systems[index]->name());
        }
        else
        {
            log::warn("The system is not installed.");
        }
    }

    static void shutdown();

    static void begin_frame();
    static void end_frame();

    template <typename T>
    static T& system() requires derived_from_system<T>
    {
        auto system_pointer = instance().m_systems[system_index::value<T>()].get();
        ASH_ASSERT(dynamic_cast<T*>(system_pointer));
        return *static_cast<T*>(system_pointer);
    }

private:
    context();
    static context& instance();

    std::map<std::string, dictionary> m_config;

    std::vector<std::unique_ptr<system_base>> m_systems;
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