#pragma once

#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class scan_pass
{
public:
    struct parameter
    {
        rdg_buffer* buffer;
        std::uint32_t size;
    };

    void add(render_graph& graph, const parameter& parameter);

private:
    static constexpr std::uint32_t group_size = 256;
    static constexpr std::uint32_t max_group_count = 16;

    void add_scan_pass(render_graph& graph, const parameter& parameter);
    void add_offset_pass(render_graph& graph, const parameter& parameter);

    rdg_buffer* m_offset_buffer{nullptr};
};
} // namespace violet