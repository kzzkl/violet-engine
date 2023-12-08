#pragma once

#include "graphics/renderer.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{
class render_node
{
public:
    render_node(std::string_view name, renderer* renderer);
    render_node(const render_node&) = delete;
    virtual ~render_node();

    render_node& operator=(const renderer&) = delete;

    const std::string& get_name() const noexcept { return m_name; }
    renderer* get_renderer() const noexcept { return m_renderer; }

private:
    std::string m_name;
    renderer* m_renderer;
};
} // namespace violet