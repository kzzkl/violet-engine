#include "graphics/rhi.hpp"
#include "common/assert.hpp"
#include "core/plugin.hpp"
#include "common/log.hpp"

namespace violet::graphics
{
class rhi_plugin : public core::plugin
{
public:
    rhi_plugin() {}

    rhi_interface& rhi_impl() { return *m_rhi_impl; }

protected:
    virtual bool on_load() override
    {
        make_rhi make = static_cast<make_rhi>(find_symbol("make_rhi"));
        if (make == nullptr)
        {
            log::error("Symbol not found in plugin: make_rhi.");
            return false;
        }

        m_rhi_impl.reset(make());

        return true;
    }

    virtual void on_unload() override { m_rhi_impl = nullptr; }

private:
    std::unique_ptr<rhi_interface> m_rhi_impl;
};

rhi& rhi::instance()
{
    static rhi instance;
    return instance;
}

void rhi::initialize(std::string_view plugin, const rhi_desc& desc)
{
    instance().m_plugin = std::make_unique<rhi_plugin>();
    instance().m_plugin->load(plugin);

    impl().initialize(desc);

    instance().m_renderer.reset(impl().make_renderer());
}

renderer_interface& rhi::renderer()
{
    return *instance().m_renderer;
}

resource_format rhi::back_buffer_format()
{
    return instance().m_renderer->back_buffer()->format();
}

std::unique_ptr<pipeline_parameter_interface> rhi::make_pipeline_parameter(
    const pipeline_parameter_desc& desc)
{
    pipeline_parameter_interface* interface = impl().make_pipeline_parameter(desc);
    return std::unique_ptr<pipeline_parameter_interface>(interface);
}

std::unique_ptr<render_pipeline_interface> rhi::make_render_pipeline(
    const render_pipeline_desc& desc)
{
    return std::unique_ptr<render_pipeline_interface>(impl().make_render_pipeline(desc));
}

std::unique_ptr<compute_pipeline_interface> rhi::make_compute_pipeline(
    const compute_pipeline_desc& desc)
{
    return std::unique_ptr<compute_pipeline_interface>(impl().make_compute_pipeline(desc));
}

std::unique_ptr<resource_interface> rhi::make_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    resource_format format)
{
    return std::unique_ptr<resource_interface>(impl().make_texture(data, width, height, format));
}

std::unique_ptr<resource_interface> rhi::make_texture(std::string_view file)
{
    return std::unique_ptr<resource_interface>(impl().make_texture(file.data()));
}

std::unique_ptr<resource_interface> rhi::make_texture_cube(
    std::string_view right,
    std::string_view left,
    std::string_view top,
    std::string_view bottom,
    std::string_view front,
    std::string_view back)
{
    return std::unique_ptr<resource_interface>(impl().make_texture_cube(
        right.data(),
        left.data(),
        top.data(),
        bottom.data(),
        front.data(),
        back.data()));
}

std::unique_ptr<resource_interface> rhi::make_shadow_map(const shadow_map_desc& desc)
{
    return std::unique_ptr<resource_interface>(impl().make_shadow_map(desc));
}

std::unique_ptr<resource_interface> rhi::make_render_target(const render_target_desc& desc)
{
    return std::unique_ptr<resource_interface>(impl().make_render_target(desc));
}

std::unique_ptr<resource_interface> rhi::make_depth_stencil_buffer(
    const depth_stencil_buffer_desc& desc)
{
    return std::unique_ptr<resource_interface>(impl().make_depth_stencil_buffer(desc));
}

rhi_interface& rhi::impl()
{
    return instance().m_plugin->rhi_impl();
}
} // namespace violet::graphics