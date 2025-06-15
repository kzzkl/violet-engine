#include "graphics/graphics_config.hpp"

namespace violet
{
graphics_config& graphics_config::instance()
{
    static graphics_config instance;
    return instance;
}

void graphics_config::initialize(
    std::uint32_t max_draw_commands,
    std::uint32_t max_candidate_clusters)
{
    m_max_draw_commands = max_draw_commands;
    m_max_candidate_clusters = max_candidate_clusters;
}
} // namespace violet