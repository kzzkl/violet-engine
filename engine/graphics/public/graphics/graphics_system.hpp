#pragma once

#include "core/engine_system.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "graphics/rhi.hpp"

namespace violet
{
class rhi_plugin;
class graphics_system : public engine_system
{
public:
    graphics_system();
    virtual ~graphics_system();

    virtual bool initialize(const dictionary& config) override;
    virtual void shutdown() override;

    void render(render_graph* graph);

    rhi_renderer* get_rhi() const;

private:
    void begin_frame();
    void end_frame();
    void render();

    std::vector<render_graph*> m_render_graphs;

    std::unique_ptr<rhi_plugin> m_plugin;

    bool m_idle;
};
} // namespace violet