#pragma once

#include "assert.hpp"
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
    using module_list = std::unordered_map<uuid, std::unique_ptr<submodule>, uuid_hash>;

public:
    context(const ash::common::dictionary& config);

    template <typename T>
    T& get_submodule()
    {
        return *static_cast<T*>(m_modules[submodule_trait<T>::id].get());
    }

    ash::task::task_manager& get_task() { return *m_task; }

protected:
    template <derived_from_submodule T, typename... Args>
    void install_submodule(Args&&... args)
    {
        uuid id = submodule_trait<T>::id;
        if (m_modules[id] == nullptr)
        {
            auto m = std::make_unique<T>(std::forward<Args>(args)...);
            m->m_context = this;
            ash::common::log::info("Module installed successfully: {}.", m->get_name());
            m_modules[id] = std::move(m);
        }
        else
        {
            ash::common::log::warn("The module is already installed.");
        }
    }

    void initialize_submodule();

private:
    ash::common::dictionary m_config;

    module_list m_modules;
    std::unique_ptr<ash::task::task_manager> m_task;
};
} // namespace ash::core