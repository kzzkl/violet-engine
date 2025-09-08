#pragma once

#include <cstdint>

namespace violet
{
class graphics_config
{
public:
    static std::uint32_t get_max_draw_command_count() noexcept
    {
        return instance().m_max_draw_command_count;
    }

    static std::uint32_t get_max_cluster_count() noexcept
    {
        return get_max_candidate_cluster_count() - get_max_cluster_node_count();
    }

    static std::uint32_t get_max_cluster_node_count() noexcept
    {
        return get_max_candidate_cluster_count() / 16;
    }

    static std::uint32_t get_max_candidate_cluster_count() noexcept
    {
        return instance().m_max_candidate_cluster_count;
    }

private:
    graphics_config() = default;

    static graphics_config& instance();

    void initialize(
        std::uint32_t max_draw_command_count,
        std::uint32_t max_candidate_cluster_count);

    std::uint32_t m_max_draw_command_count;
    std::uint32_t m_max_candidate_cluster_count;

    friend class graphics_system;
};
} // namespace violet