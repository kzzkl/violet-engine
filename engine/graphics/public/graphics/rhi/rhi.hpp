#pragma once

#include "graphics_interface.hpp"
#include <memory>
#include <string>

namespace violet
{
class rhi_plugin;
class rhi
{
public:
    rhi(std::string_view plugin_path, const rhi_desc& desc);
    ~rhi();

    std::unique_ptr<render_pipeline_interface> make_render_pipeline(
        const render_pipeline_desc& desc) const;
    std::unique_ptr<compute_pipeline_interface> make_compute_pipeline(
        const compute_pipeline_desc& desc) const;
    std::unique_ptr<pipeline_parameter_interface> make_pipeline_parameter(
        const pipeline_parameter_desc& desc) const;

    renderer_interface* get_renderer() const noexcept;

private:
    std::unique_ptr<rhi_plugin> m_plugin;

    std::unique_ptr<renderer_interface> m_renderer;
};
} // namespace violet