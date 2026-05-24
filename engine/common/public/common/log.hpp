#pragma once

#include <memory>
#include <spdlog/logger.h>

namespace violet
{
class log
{
public:
    static void initialize(std::string_view file_path);

    template <typename... Args>
    static void error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        instance().m_logger->error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        instance().m_logger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        instance().m_logger->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        instance().m_logger->debug(fmt, std::forward<Args>(args)...);
    }

private:
    log();
    ~log();

    static log& instance();

    std::shared_ptr<spdlog::logger> m_logger;
};
} // namespace violet