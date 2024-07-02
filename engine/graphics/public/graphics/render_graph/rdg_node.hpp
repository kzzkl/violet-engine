#pragma once

#include <string>

namespace violet
{
class rdg_node
{
public:
    virtual ~rdg_node() = default;

    const std::string& get_name() const noexcept { return m_name; }
    std::size_t get_index() const noexcept { return m_index; }

private:
    std::string m_name;
    std::size_t m_index;

    friend class render_graph;
};
} // namespace violet