#pragma once

#include "graphics/rhi.hpp"
#include <string>

namespace violet
{
class render_node
{
public:
    render_node(std::string_view name, rhi_context* rhi) : m_name(name), m_rhi(rhi) {}
    virtual ~render_node() = default;

    const std::string& get_name() const noexcept { return m_name; }

    rhi_context* get_rhi() const noexcept { return m_rhi; }

private:
    std::string m_name;
    rhi_context* m_rhi;
};
} // namespace violet