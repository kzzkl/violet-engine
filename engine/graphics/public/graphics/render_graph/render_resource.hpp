#pragma once

#include "graphics/render_graph/render_node.hpp"

namespace violet
{
class render_resource : public render_node
{
public:
    render_resource(std::string_view name, rhi_context* rhi);

    void set_format(rhi_resource_format format) noexcept { m_format = format; }
    rhi_resource_format get_format() const noexcept { return m_format; }

    void set_resource(rhi_resource* resource) noexcept { m_resource = resource; }
    rhi_resource* get_resource() const noexcept { return m_resource; }

private:
    rhi_resource_format m_format;
    rhi_resource* m_resource;
};
} // namespace violet