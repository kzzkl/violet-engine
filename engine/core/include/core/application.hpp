#pragma once

#include "core/context.hpp"

namespace violet::core
{
class application
{
public:
    application(std::string_view config_path = "");
    application(const application&) = delete;

    template <derived_from_system T, typename... Args>
    void install(Args&&... args)
    {
        context::install<T>(std::forward<Args>(args)...);
    }

    void run();
    void exit();

    application& operator=(const application&) = delete;

private:
    std::atomic<bool> m_exit;
};
} // namespace violet::core