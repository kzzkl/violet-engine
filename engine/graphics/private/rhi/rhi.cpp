#include "graphics/rhi/rhi.hpp"
#include "rhi/rhi_plugin.hpp"

namespace violet
{
rhi::rhi(std::string_view plugin_path, const rhi_desc& desc)
{
    m_plugin = std::make_unique<rhi_plugin>();
    m_plugin->load(plugin_path);

    rhi_interface* impl = m_plugin->get_rhi_impl();
    impl->initialize(desc);
    m_renderer.reset(impl->make_renderer());
}

rhi::~rhi()
{
}

std::unique_ptr<render_pipeline_interface> rhi::make_render_pipeline(
    const render_pipeline_desc& desc) const
{
    rhi_interface* impl = m_plugin->get_rhi_impl();
    return std::unique_ptr<render_pipeline_interface>(impl->make_render_pipeline(desc));
}

std::unique_ptr<compute_pipeline_interface> rhi::make_compute_pipeline(
    const compute_pipeline_desc& desc) const
{
    rhi_interface* impl = m_plugin->get_rhi_impl();
    return std::unique_ptr<compute_pipeline_interface>(impl->make_compute_pipeline(desc));
}

std::unique_ptr<pipeline_parameter_interface> rhi::make_pipeline_parameter(
    const pipeline_parameter_desc& desc) const
{
    rhi_interface* impl = m_plugin->get_rhi_impl();
    return std::unique_ptr<pipeline_parameter_interface>(impl->make_pipeline_parameter(desc));
}

renderer_interface* rhi::get_renderer() const noexcept
{
    return m_renderer.get();
}
} // namespace violet