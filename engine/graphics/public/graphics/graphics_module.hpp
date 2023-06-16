#pragma once

#include "core/context/engine_module.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "graphics/rhi.hpp"

namespace violet
{
class rhi_plugin;
class graphics_module : public engine_module
{
public:
    graphics_module();
    virtual ~graphics_module();

    virtual bool initialize(const dictionary& config) override;

    render_graph* get_render_graph() noexcept { return m_render_graph.get(); }
    rhi_context* get_rhi() const;

private:
    void render();

    std::unique_ptr<render_graph> m_render_graph;

    std::unique_ptr<rhi_plugin> m_plugin;
};
} // namespace violet