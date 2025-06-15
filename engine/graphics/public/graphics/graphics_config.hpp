#pragma once

#include <cstdint>

namespace violet
{
class graphics_config
{
public:
    static std::uint32_t get_max_draw_commands() noexcept
    {
        return instance().m_max_draw_commands;
    }

    static std::uint32_t get_max_candidate_clusters() noexcept
    {
        return instance().m_max_candidate_clusters;
    }

private:
    graphics_config() = default;

    static graphics_config& instance();

    void initialize(std::uint32_t max_draw_commands, std::uint32_t max_candidate_clusters);

    std::uint32_t m_max_draw_commands;
    std::uint32_t m_max_candidate_clusters;

    friend class graphics_system;
};
} // namespace violet