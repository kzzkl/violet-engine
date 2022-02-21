#pragma once

#include "spdlog/fmt/ostr.h"
#include "spdlog/logger.h"
#include <memory>
#include <string_view>

#include "common_exports.hpp"

namespace ash::common
{
class COMMON_API log
{
public:
    template <typename... Param>
    static void error(std::string_view format, const Param&... params)
    {
        instance().m_logger->error(format, params...);
    }

    template <typename... Param>
    static void warn(std::string_view format, const Param&... params)
    {
        instance().m_logger->warn(format, params...);
    }

    template <typename... Param>
    static void info(std::string_view format, const Param&... params)
    {
        instance().m_logger->info(format, params...);
    }

    template <typename... Param>
    static void debug(std::string_view format, const Param&... params)
    {
        instance().m_logger->debug(format, params...);
    }

private:
    log();
    ~log();

    static log& instance();

    std::shared_ptr<spdlog::logger> m_logger;
};
} // namespace ash::common