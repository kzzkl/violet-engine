#pragma once

#include "core_exports.hpp"
#include "dictionary.hpp"
#include "log.hpp"
#include "submodule.hpp"
#include "task_manager.hpp"
#include <memory>
#include <vector>

namespace ash::core
{
class CORE_API context
{
public:
    context(const ash::common::dictionary& config);

    template <typename T>
    T& get_submodule()
    {
        ASH_ASSERT(m_modules[submodule_trait<T>::index()] != nullptr, "Module not installed.");
        return *m_modules[submodule_trait<T>::index()];
    }

    ash::task::task_manager& get_task() { return *m_task; }

protected:
    template <derived_from_submodule T, typename... Args>
    void install_submodule(Args&&... args)
    {
        submodule_index index = submodule_trait<T>::index();
        if (m_modules.size() < index + 1)
            m_modules.resize(index + 1);

        if (m_modules[index] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->m_context = this;
            ash::common::log::info("Module installed successfully: {}.", m->get_name());
            m_modules[index] = std::move(m);
        }
        else
        {
            ash::common::log::warn("The module is already installed.");
        }
    }

    void initialize_submodule();

private:
    ash::common::dictionary m_config;

    std::vector<std::unique_ptr<submodule>> m_modules;
    std::unique_ptr<ash::task::task_manager> m_task;
};
} // namespace ash::core