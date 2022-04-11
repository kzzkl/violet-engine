#pragma once

#include "context.hpp"

namespace ash::core
{
class application : public context
{
public:
    application(std::string_view config_path = "config");
    application(const application&) = delete;

    template <derived_from_submodule T, typename... Args>
    void install(Args&&... args)
    {
        install_submodule<T>(std::forward<Args>(args)...);
    }

    void run();
    void exit();

    application& operator=(const application&) = delete;

private:
    std::atomic<bool> m_exit;
};
} // namespace ash::core