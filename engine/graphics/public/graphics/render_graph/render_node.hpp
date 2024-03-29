#pragma once

#include "graphics/graphics_context.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{
class render_node
{
public:
    render_node(std::string_view name, graphics_context* context);
    render_node(const render_node&) = delete;
    virtual ~render_node();

    render_node& operator=(const graphics_context&) = delete;

    const std::string& get_name() const noexcept { return m_name; }
    graphics_context* get_context() const noexcept { return m_context; }

private:
    std::string m_name;
    graphics_context* m_context;
};
} // namespace violet