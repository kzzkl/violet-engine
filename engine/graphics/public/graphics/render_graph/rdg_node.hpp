#pragma once

#include <string>

namespace violet
{
class rdg_node
{
public:
    rdg_node() = default;
    rdg_node(const rdg_node&) = delete;
    virtual ~rdg_node() = default;

    rdg_node& operator=(const rdg_node&) = delete;

    const std::string& get_name() const noexcept
    {
        return m_name;
    }

    std::size_t get_index() const noexcept
    {
        return m_index;
    }

    std::size_t get_batch() const noexcept
    {
        return m_batch;
    }

private:
    std::string m_name;
    std::size_t m_index;
    std::size_t m_batch;

    friend class render_graph;
};
} // namespace violet