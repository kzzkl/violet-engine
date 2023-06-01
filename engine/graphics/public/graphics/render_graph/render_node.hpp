#pragma once

#include <string>

namespace violet
{
class render_node
{
public:
    render_node(std::string_view name, std::size_t index) : m_name(name), m_index(index) {}
    virtual ~render_node() = default;

    virtual bool compile() = 0;

    const std::string& get_name() const noexcept { return m_name; }
    std::size_t get_index() const noexcept { return m_index; }

private:
    std::string m_name;
    std::size_t m_index;
};
} // namespace violet