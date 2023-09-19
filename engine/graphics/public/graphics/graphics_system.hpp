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

    void add_pipeline_parameter_layout(
        std::string_view name,
        const rhi_pipeline_parameter_layout_desc& desc);
    rhi_pipeline_parameter_layout* get_pipeline_parameter_layout(std::string_view name) const
    {
        auto iter = m_pipeline_parameter_layouts.find(name.data());
        return iter->second;
    }

    rhi_context* get_rhi() const;

private:
    void begin_frame();
    void end_frame();
    void render();

    std::vector<render_graph*> m_render_graphs;
    std::unique_ptr<rhi_plugin> m_plugin;

    std::map<std::string, rhi_pipeline_parameter_layout*> m_pipeline_parameter_layouts;

    bool m_idle;
};
} // namespace violet