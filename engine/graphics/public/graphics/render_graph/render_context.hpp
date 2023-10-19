#pragma once

#include "graphics/render_interface.hpp"
#include <map>
#include <string>
#include <vector>

namespace violet
{
class render_context
{
public:
    render_context(rhi_renderer* rhi);
    render_context(const render_context&) = delete;
    ~render_context();

    rhi_parameter_layout* add_parameter_layout(
        std::string_view name,
        const std::vector<std::pair<rhi_parameter_type, std::size_t>>& layout);
    rhi_parameter_layout* get_parameter_layout(std::string_view name) const;

    rhi_renderer* get_rhi() const noexcept { return m_rhi; }

    render_context& operator=(const render_context&) = delete;

private:
    rhi_renderer* m_rhi;

    std::map<std::string, rhi_parameter_layout*> m_parameter_layouts;
};
} // namespace violet