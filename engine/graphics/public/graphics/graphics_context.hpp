#pragma once

#include "graphics/render_interface.hpp"
#include <map>
#include <string>
#include <vector>

namespace violet
{
class graphics_context
{
public:
    graphics_context(rhi_renderer* rhi);
    graphics_context(const graphics_context&) = delete;
    ~graphics_context();

    rhi_parameter_layout* add_parameter_layout(
        std::string_view name,
        const std::vector<rhi_parameter_layout_pair>& layout);
    rhi_parameter_layout* get_parameter_layout(std::string_view name) const;

    rhi_renderer* get_rhi() const noexcept { return m_rhi; }

    graphics_context& operator=(const graphics_context&) = delete;

private:
    rhi_renderer* m_rhi;

    std::map<std::string, rhi_parameter_layout*> m_parameter_layouts;
};
} // namespace violet