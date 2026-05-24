#include "common/log.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

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

void log::initialize(std::string_view file_path)
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("%T.%e %^%-5l%$ | %v");

    std::vector<spdlog::sink_ptr> sinks = {console_sink};

    if (!file_path.empty())
    {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path.data());
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("%T.%e %^%-5l%$ | %v");
        file_sink->should_log(spdlog::level::trace);

        sinks.push_back(file_sink);
    }

    instance().m_logger =
        std::make_shared<spdlog::logger>("multi_logger", sinks.begin(), sinks.end());

    instance().m_logger->flush_on(spdlog::level::info);
}
} // namespace violet