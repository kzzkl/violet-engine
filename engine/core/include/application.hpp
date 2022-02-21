#pragma once

#include "assert.hpp"
#include "core_exports.hpp"
#include "dictionary.hpp"
#include "log.hpp"
#include "submodule.hpp"
#include <memory>
#include <vector>

namespace ash::core
{
class CORE_API application
{
public:
    application(const ash::common::dictionary& config);
    application(const application&) = delete;

    template <typename T, typename... Argv>
    void install(Argv&&... argv)
    {
        submodule_index index = submodule_trait<T>::index();
        if (m_modules.size() < index + 1)
            m_modules.resize(index + 1);

        if (m_modules[index] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Argv>(argv)...);
            ash::common::log::info("Module installed successfully: {}", m->get_name());
            m_modules[index] = std::move(m);
        }
        else
        {
            ash::common::log::warn("The module is already installed");
        }
    }

    template <typename T>
    T& get_submodule()
    {
        ASH_ASSERT(m_modules[submodule_trait<T>::index()] != nullptr, "Module not installed");
        return *m_modules[submodule_trait<T>::index()];
    }

    void run();

    application& operator=(const application&) = delete;

private:
    ash::common::dictionary m_config;
    std::vector<std::unique_ptr<submodule>> m_modules;
};
} // namespace ash::core