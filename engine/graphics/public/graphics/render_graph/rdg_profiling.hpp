#pragma once

#include "graphics/render_device.hpp"
#include <stack>
#include <string>
#include <vector>

namespace violet
{
class rdg_profiling
{
public:
    struct node
    {
        std::string name;
        float time_ms;
        std::uint32_t query_index;
        std::vector<std::uint32_t> children;
    };

    rdg_profiling();

    void begin(std::string_view name, std::uint32_t query_index = 0xFFFFFFFF);
    void end();

    void reset(std::uint32_t leaf_count);
    void resolve();

    const std::vector<node>& get_nodes() const noexcept
    {
        return m_nodes;
    }

    rhi_query_pool* get_query_pool() const;

private:
    float get_node_time(std::uint32_t node_index);

    std::vector<node> m_nodes;
    std::stack<std::uint32_t> m_node_stack;

    std::vector<rhi_ptr<rhi_query_pool>> m_query_pools;
    std::uint32_t m_query_pool_index{0};

    std::uint32_t m_leaf_count{0};

    bool m_resolved{false};
};
} // namespace violet