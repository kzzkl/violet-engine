#include "graphics/graphics_config.hpp"

namespace violet
{
graphics_config& graphics_config::instance()
{
    static graphics_config instance;
    return instance;
}

void graphics_config::initialize(
    std::uint32_t max_draw_command_count,
    std::uint32_t max_candidate_cluster_count)
{
    m_max_draw_command_count = max_draw_command_count;
    m_max_candidate_cluster_count = max_candidate_cluster_count;
}
} // namespace violet