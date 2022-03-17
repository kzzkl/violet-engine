#pragma once

#include "common_exports.hpp"
#include "spdlog/fmt/ostr.h"
#include "spdlog/logger.h"
#include <memory>
#include <string_view>

namespace ash
{
class COMMON_API log
{
public:
    template <typename... Args>
    static void error(std::string_view format, const Args&... args)
    {
        instance().m_logger->error(format, args...);
    }

    template <typename... Args>
    static void warn(std::string_view format, const Args&... args)
    {
        instance().m_logger->warn(format, args...);
    }

    template <typename... Args>
    static void info(std::string_view format, const Args&... args)
    {
        instance().m_logger->info(format, args...);
    }

    template <typename... Args>
    static void debug(std::string_view format, const Args&... args)
    {
        instance().m_logger->debug(format, args...);
    }

private:
    log();
    ~log();

    static log& instance();

    std::shared_ptr<spdlog::logger> m_logger;
};
} // namespace ash