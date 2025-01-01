#include "common/log.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace violet
{
log::log()
{
    m_logger = spdlog::stdout_color_mt("console");
    m_logger->set_level(spdlog::level::trace);
    m_logger->set_pattern("%T.%e %^%-5l%$ | %v");
}

log::~log()
{
    spdlog::drop_all();
}

log& log::instance()
{
    static log instance;
    return instance;
}
} // namespace violet